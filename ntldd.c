/*
    ntldd - lists dynamic dependencies of a module

    Copyright (C) 2010 LRN

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
Code is mostly written after
"An In-Depth Look into the Win32 Portable Executable File Format"
MSDN Magazine articles
*/

#include <windows.h>

#include <Imagehlp.h>

#include <winnt.h>

#include <string.h>
#include <stdio.h>

#include "libntldd.h"

void printversion()
{
  printf ("ntldd %d.%d\n\
Copyright (C) 2010 LRN\n\
This is free software; see the source for conditions. There is NO\n\
warranty; not event for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
Written by LRN.", NTLDD_VERSION_MAJOR, NTLDD_VERSION_MINOR);
}

void printhelp(char *argv0)
{
  printf("Usage: %s [OPTION]... FILE...\n\
OPTIONS:\n\
--version         Displays version\n\
-v, --verbose         Does not work\n\
-u, --unused          Does not work\n\
-d, --data-relocs     Does not work\n\
-r, --function-relocs Does not work\n\
-R, --recursive       Lists dependencies recursively,\n\
                        eliminating duplicates\n\
--help                Displays this message\n\
\n\
Use -- option to pass filenames that start with `--' or `-'\n\
For bug reporting instructions, please see:\n\
<somewhere>.", argv0);
}

int PrintImageLinks (int first, int verbose, int unused, int datarelocs, int functionrelocs, struct DepTreeElement *self, int recursive, int depth)
{
  int i;
  self->flags |= DEPTREE_VISITED;

  if (self->flags & DEPTREE_UNRESOLVED)  
  {
    if (!first)
      printf (" => not found\n");
    else
      fprintf (stderr, "%s: not found\n", self->module);
    return -1;
  }

  if (!first)
  {
    if (stricmp (self->module, self->resolved_module) == 0)
      printf (" (0x%p)\n", self->mapped_address);
    else
      printf (" => %s (0x%p)\n", self->resolved_module,
          self->mapped_address);
  }

  if (first || recursive)
  {
    for (i = 0; i < self->childs_len; i++)
    {
      if (!(self->childs[i]->flags & DEPTREE_VISITED))
      {
        printf ("\t%*s%s", depth, depth > 0 ? " " : "", self->childs[i]->module);
        PrintImageLinks (0, verbose, unused, datarelocs, functionrelocs, self->childs[i], recursive, depth + 1);
      }
    }
  }
  return 0;
}

int main (int argc, char **argv)
{
  int i;
  int verbose = 0;
  int unused = 0;
  int datarelocs = 0;
  int functionrelocs = 0;
  int skip = 0;
  int files = 0;
  int recursive = 0;
  int files_start = -1;
  for (i = 1; i < argc; i++)
  {
    if (strcmp (argv[i], "--version") == 0)
      printversion ();
    else if (strcmp (argv[i], "-v") == 0 || strcmp (argv[i], "--verbose") == 0)
      verbose = 1;
    else if (strcmp (argv[i], "-u") == 0 || strcmp (argv[i], "--unused") == 0)
      unused = 1;
    else if (strcmp (argv[i], "-d") == 0 || 
        strcmp (argv[i], "--data-relocs") == 0)
      datarelocs = 1;
    else if (strcmp (argv[i], "-r") == 0 || 
        strcmp (argv[i], "--function-relocs") == 0)
      functionrelocs = 1;
    else if (strcmp (argv[i], "-R") == 0 || 
        strcmp (argv[i], "--recursive") == 0)
      recursive = 1;
    else if (strcmp (argv[i], "--help") == 0)
    {
      printhelp (argv[0]);
      skip = 1;
      break;
    }
    else if (strcmp (argv[i], "--") == 0)
    {
      files = 1;
    }
    else if (strlen (argv[i]) > 1 && argv[i][0] == '-' && (argv[i][1] == '-' ||
        strlen (argv[i]) == 2) && !files)
    {
      fprintf (stderr, "Unrecognized option `%s'\n\
Try `ntldd --help' for more information\n", argv[i]);
      skip = 1;
      break;
    }
    else if (files_start < 0)
    {
      skip = 0;
      files_start = i;
      break;
    }
  }
  if (!skip && files_start > 0)
  {
    int multiple = files_start + 1 < argc;
    struct DepTreeElement root;
    memset (&root, 0, sizeof (struct DepTreeElement));
    for (i = files_start; i < argc; i++)
    {
      struct DepTreeElement *child = (struct DepTreeElement *) malloc (sizeof (struct DepTreeElement));
      memset (child, 0, sizeof (struct DepTreeElement));
      child->module = strdup (argv[i]);
      AddDep (&root, child);
      BuildDepTree (datarelocs, functionrelocs, argv[i], recursive, &root, child, 0);
    }
    ClearDepStatus (&root, DEPTREE_VISITED | DEPTREE_PROCESSED);
    for (i = files_start; i < argc; i++)
    {
      if (multiple)
        printf ("%s:\n", argv[i]);
      PrintImageLinks (1, verbose, unused, datarelocs, functionrelocs, root.childs[i - files_start], recursive, 0);
    }
  }
  return 0;
}