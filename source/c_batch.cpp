// Emacs style mode select -*- C++ -*- vi:sw=3 ts=3:
//----------------------------------------------------------------------------
//
// Copyright(C) 2012 Charles Gunyon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//   Batched console commands.
//
//----------------------------------------------------------------------------

#include "z_zone.h"

#include "c_batch.h"
#include "c_runcmd.h"
#include "e_hash.h"

class CommandBatches : public ZoneObject
{

private:
   const static short initial_batch_count = 31;
   const static float load_factor = 0.7f;

   EHashTable<CommandBatch, ENCStringHashKey, &CommandBatch::key,
              &CommandBatch::links> command_batches;

   DLListItem<CommandBatch> *finished_batches;

public:
   CommandBatches()
      : ZoneObject(), finished_batches(NULL)
   {
      command_batches.initialize(initial_batch_count);
   }

   void addBatch(const char *name, const char *commands)
   {
      CommandBatch *batch = new CommandBatch(name, commands);

      command_batches.addObject(batch);
      if(command_batches.getLoadFactor() > load_factor)
         command_batches.rebuild(command_batches.getNumChains() * 2);
   }

   CommandBatch* getBatch(const char *name)
   {
      return command_batches.objectForKey(name);
   }

   void removeBatch(CommandBatch *batch)
   {
      command_batches.removeObject(batch);
   }

   CommandBatch* batchIterator(CommandBatch *batch)
   {
      return command_batches.tableIterator(batch);
   }

   void Tick()
   {
      CommandBatch *batch = NULL;

      // [CG] Clean up finished batches first.
      while((batch = command_batches.tableIterator(batch)))
      {
         if(!batch->isFinished())
            continue;

         batch->finished_links.insert(batch, &finished_batches);
      }

      while(finished_batches)
      {
         batch = finished_batches->dllObject;
         finished_batches->remove();

         command_batches.removeObject(batch);
         delete(batch);
      }

      // [CG] Finally run any remaining batches.
      batch = NULL;
      while((batch = command_batches.tableIterator(batch)))
         batch->run();
   }
};

static CommandBatches command_batches;
static CommandBatches saved_command_batches;

CommandBatch::CommandBatch(const char *new_name, const char *new_commands)
   : ZoneObject(), delay(0), finished(false)
{
   name = estrdup(new_name);
   commands = estrdup(new_commands);
   current_command = commands;
   key = name;
}

CommandBatch::~CommandBatch()
{
   if(name)
      efree(name);

   if(commands)
      efree(commands);
}

const char* CommandBatch::getName() const
{
   return name;
}

const char* CommandBatch::getCommands() const
{
   return commands;
}

bool CommandBatch::isFinished()
{
   if(finished)
      return true;

   return false;
}

unsigned int CommandBatch::parseWait(const char *token)
{
   int wait;

   if(strncasecmp(token, "wait ", 5))
      return 0;

   wait = atoi((token + 5));

   if(wait > 0)
      return wait;

   return 0;
}

void CommandBatch::run()
{
   char *next_token;
   unsigned int wait;

   if(delay)
   {
      delay--;
      return;
   }

   // [CG] Process all commands until we reach a wait or the end.
   while(current_command)
   {
      next_token = strchr(current_command, ';');

      if(next_token)
      {
         // [CG] Build up the command buffer.
         command_buffer.clear();
         while(current_command != next_token)
         {
            command_buffer += (*current_command);
            current_command++;
         }

         // [CG] Check for a wait command.
         if((wait = parseWait(command_buffer.constPtr())))
         {
            delay += wait;
            break;
         }
         else if(command_buffer.length())
            C_RunTextCmd(command_buffer.constPtr());

         current_command++;
      }
      else
      {
         // [CG] Don't process trailing wait commands.
         if((!parseWait(current_command)) && command_buffer.length())
            C_RunTextCmd(current_command);

         finished = true;

         current_command = NULL;
      }
   }
}

void C_ActivateCommandBatch()
{
   const char *name, *commands;

   name = Console.command->name;
   commands = (const char *)(Console.command->variable->variable);
   command_batches.addBatch(name, commands);
}

void C_AddCommandBatch(const char *name, const char *commands)
{
   CommandBatch *batch = NULL;
   command_t *command = C_GetCmdForName(name);
   variable_t *variable = estructalloc(variable_t, 1);

   variable->variable = estrdup(commands);
   variable->v_default = NULL;
   variable->type = vt_string;
   variable->min = 0;
   variable->max = 128;
   variable->defines = NULL;

   if(command)
   {
      efree(command->variable->variable);
      efree(command->variable);
      command->variable = variable;
   }
   else
   {
      command = estructalloc(command_t, 1);
      command->name = strdup(name);
      command->type = ct_command;
      command->flags = 0;
      command->variable = variable;
      command->handler = C_ActivateCommandBatch;
      command->netcmd = 0;

      (C_AddCommand)(command);
   }

   if((batch = saved_command_batches.getBatch(name)))
      saved_command_batches.removeBatch(batch);

   saved_command_batches.addBatch(name, commands);
}

const char* C_GetCommandBatch(const char *name)
{
   command_t *batch = C_GetCmdForName(name);

   if(!batch)
      return NULL;

   return (const char *)batch->variable->variable;
}

void C_CommandBatchTicker()
{
   command_batches.Tick();
}

CommandBatch* C_CommandBatchIterator(CommandBatch *batch)
{
   return saved_command_batches.batchIterator(batch);
}
