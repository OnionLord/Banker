Banker
======

Banker's Algorithm

Developers: Byron A. Craig & Ryan Robertson
Institution: James Madison University, 2013

Description:
------------

To run these files you need to use 'gcc' to compile and execute. These files altogether are very large and may
be hard to follow, but I included MANY comments to hopefully keep you from being confused. These programming 
files use InterProcess Communication and the files *banker.c* and *client.c* MUST BE RUN in two separate command
line windows. Below I explain how to run these files.

I would not recommend running them in TABS because it will be hard to keep track of what is going on as it runs
very fast. Other than that, have fun testing these files!

======================================================================================================
*ALSO, THIS PROGRAM IS A WORK-IN-PROGRESS AND THERE ARE STILL SMALL ISSUES THAT NEED TO BE SORTED OUT*
======================================================================================================

How To Run:
-----------
Open one terminal window that will run *banker.c*
Open another terminal window that will run *client.c*
NOTE: If you want to run more than one client, you have to open another terminal window for each extra client.

Commands
- gcc -o banker banker.c (SEPERATE WINDOW)
- gcc -o client client.c (SEPARATE WINDOW)
- ./banker (RUN THIS FILE FIRST)
- ./client # ('#' is what you want the client number to be, for example: ./client 1)

Issues:
-------
- Math seems to be calculating more than it should when more than 1 client is requesting resources, making the 
  output values too big.
- Print output formatting may at times be buggy.
