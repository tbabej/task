////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2006 - 2015, Paul Beckingham, Federico Hernandez.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// http://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////

#include <cmake.h>
#include <stdlib.h>
#include <Context.h>
#include <i18n.h>
#include <text.h>
#include <CmdExec.h>

extern Context context;

////////////////////////////////////////////////////////////////////////////////
CmdExec::CmdExec ()
{
  _keyword               = "execute";
  _usage                 = "task          execute <external command>";
  _description           = STRING_CMD_EXEC_USAGE;
  _read_only             = true;
  _displays_id           = false;
  _needs_gc              = false;
  _uses_context          = false;
  _accepts_filter        = true;
  _accepts_modifications = false;
  _accepts_miscellaneous = true;
  _category              = Command::Category::misc;
}

////////////////////////////////////////////////////////////////////////////////
int CmdExec::execute (std::string& output)
{
  std::string args;
  std::string filter;
  std::string command_line;

  std::vector <std::string> filterWords;
  std::vector <std::string> argsWords;

  for (auto& a : context.cli2._args)
    if (a.hasTag ("ORIGINAL"))
    {
      if (a.hasTag ("FILTER"))
        filterWords.push_back (a.attribute ("raw"));
      else if (a.hasTag ("MISCELLANEOUS"))
        argsWords.push_back (a.attribute ("raw"));
    }

  join (args, " ", argsWords);
  join (filter, " ", filterWords);
  join (command_line, " ", context.cli2.getWords ());

  std::string filterEnv = std::string("TASK_FILTER=") + filter;
  std::string argsEnv = std::string("TASK_ARGS=") + args;

  putenv(const_cast<char*> (filterEnv.c_str ()));
  putenv(const_cast<char*> (argsEnv.c_str ()));

  return system (command_line.c_str ());
}

////////////////////////////////////////////////////////////////////////////////
