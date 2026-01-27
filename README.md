Unipi OS Configurator
=====================

Package containing binary tools which detect connected HW and alter the behaviour of the OS 

- changing device tree overlays
- setting udev rules
- enabling / disabling daemons
- configure other applications

The package needs a **unipi-os-configurator-data** which is build separately for each product. Therefor the same package, but with different content and version is in the product-specific repository 

Package contains script os-configurator and some helpers like unipihostname, unipiid, uhelper. 

**Os-configurator** is designed to be started from systemd in early phase of boot process. It checks for hw configuration changes and if detects some then it collects informations about hw into environment variables and calls program run-parts in directory **/usr/lib/unipi/run.d**. Here are placed various scripts to modify system configuration. Os-configurator can eventually reboot the machine. This system is useful for deploying universal os images to various models of Unipi controllers. 

Packages or users can place their own scripts into directory /usr/lib/unipi/run.d to response to hw attributes collected by os-configurator. Example of such script is in doc. 
