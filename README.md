# gpu_speed_check
A little tool to check if your GPU is operating at max link width and PCIe generation. Version that acceses PCIe config space directly.

## Why? What it does?

Apparently some platforms have problem negotiating full PCIe 4.0 link with certain Nvidia RTX4090 cards and fall back to 1.1 speeds. As a result GPU performance is non existent. This happens at random on some boots.

I've got tired of having to remember to launch something like GPU-Z each time to check. So I wrote a simple tool that will display a system tray toast when GPU link is slower than card capability.
It was also a nice into to how PCI Express standard works under the hood and how to access raw config space data under modern Windows (8.1 and up).

## How to use

Put the .exe in any known folder.
Create new task in Windows Task Scheduler with trigger _At logon: Any user_. Select _Start a program_ as an action pointing to the .exe and passing `-q` as a parameter.
In _General_ tab tick _Run with highest privileges_

To test you may omit `-q` parameter and manualy launch task from Task Scheduler. Console window with GPU link parameter should appear, same as when you launch .exe directly.
