# ScreenParser
The main executable program; crunchypack core.

# What this Project Does:
This is the main crunchypack executable program. It takes a stencilpack and processes the combination of template coordinates, watchobjects, trigger scripts, action scripts, and unique image pixels and generates a symbolic in-program representation of each of these tokens and enqueues them for execution against their criteria. If the trigger script conditions evaluate to true then the associated action script string is enqueue'd in a priority queue. After each watchobject in the loop has been processed for the round then each string loaded into the priority queue is popped according to its priority and the string is output to the shell terminal. From here, the string can be pipelined to another program for injecting keystrokes or otherwise handling the event. 

# How to Install It:
Run the Windows .exe file and follow the launch instructions in the terminal. Requires a valid stencilpack file. 

# Example Usage:
I want to know the average number of times a machine learning algorithm playing a video game dies. Video games don't provide an API to extract this data and when it dies I need to dump the pre-death information to a log file. Create a stencilpack with an image 0 that represents the color of a fixed point on a "dead" screen and a watchpoint at a constant point, like a normal interface point like inventory area that turns red upon player death. Then launch the screenparser and locate the handle for the games graphical window and input the stencilpack. When this watchpoint evaluates to TRUE for being image 0, the attached argument "restart" will be output to the shell terminal and pipelined to a user-defined program. This program can handle restart with user defined and highly customizable behavior ranging from restarting the game to incrementing a death counter and writing to the log file. 


# Setting up the Development Enviornment:
Install Visusl Studio 2015, available for free, from Microsoft. It contains all the libraries like Win32 and Microsoft foundation classes needed to build the program. The program requires Win32 as it interfaces with screen objects via the Win32 API. 


# Submitting Changes:
Submit all changes in plaintext with a file name and line numbers to project lead: james@leppek.us

