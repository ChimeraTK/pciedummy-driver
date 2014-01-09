A test for the dummy driver module. Reqires the kernel module to be loaded. It accesses the /dev/utcadummys0 device file.

The test will open the device, so you can see the register content using
> cat /proc/utcadummy

After the test is finished all devices are closed and the registers are gone from /proc/utcadummy.
