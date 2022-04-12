# MTCA dummy kernel driver

The driver provides an interface compatible to the [pcieuni driver](https://github.com/ChimeraTK/pcieuni-driver) and can be used in tests instead of real hardwarte

It provides 7 devices
* mtcadummys0 to mtcadummys3
* llrfdummys4
* noioctldummys5
* pcieunidummys6

All provide 32 32bit registers in bar 0 and 4096 bytes of DMA data in bar 2. 

## Differences between the devices

TBD


## Special registers

All devices provide a rudimentary emulation of firmware in bar 0:
Register name | Address | Description
------------- | --------|-----
WORD_FIRMWARE |  0x00000000 | Major version of the kernel module, ro
WORD_COMPILATION | 0x00000004 | Minor version of the kernel module, ro
WORD_STATUS | 0x00000008 | User status register, rw
WORD_USER | 0x0000000C | User register, rw
WORD_CLK_CNT | 0x00000010 | 1D register with 2 words 
WORD_CLK_CNT_0 | 0x00000010
WORD_CLK_CNT_1 | 0x00000014
WORD_CLK_MUX | 0x00000020 | 1D register with 4 words
WORD_CLK_MUX_0 | 0x00000020
WORD_CLK_MUX_1 | 0x00000024
WORD_CLK_MUX_2 | 0x00000028
WORD_CLK_MUX_3 | 0x0000002C
WORD_DUMMY | 0x0000003C | Contains "DMMY" as ASCII, ro
WORD_CLK_RST | 0x00000040 | Clears all values in WORD_CLK_CNT and WORD_CLK_MUX if set to 1, rw
WORD_ADC_ENA | 0x00000044 | Trigger generation of a parabola in the DMA data  if set to 1, rw
BROKEN_REGISTER | 0x00000048 | Fails read and write
BROKEN_WRITE | 0x0000004C | Fails write

### SPI handshaking emulation
These three registers simulate the SPI handshaing behavior as used e.g. in [MotorDriverCard](https://github.com/ChimeraTK/MotorDriverCard)

Register name | Address | Description
------------- | --------|-----
WORD_SPI_WRITE | 0x00000050 | Write data if SPI_SYNC was set to SYNC_REQUESTED (0xFF). 
WORD_SPI_READ | 0x00000054 | Set to SYNC_ERROR (0xAA) if SPI_SYNC was not set to SYNC_REQUESTED or if the driver is configured to fail SPI. Otherwise will contain what was written to WORD_SPI_WRITE
WORD_SPI_SYNC | 0x00000058 | Ret to SYNC_REQUESTED (0xFF) before writing to WORD_SPI_WRITE for a successful write.

## Querying the current state
```
Device 0 not in use.

Device 1 not in use.

Device 2 not in use.

Device 3 not in use.

Device 4 not in use.

Device 5 not in use.

Device 6 System Bar:
Register Content_hex Content_dec:
0x00000000	0x00000000	0
0x00000001	0x00000009	9
0x00000002	0x00000000	0
0x00000003	0x00000000	0
0x00000004	0x00000019	25
0x00000005	0x00000000	0
0x00000006	0x00000000	0
0x00000007	0x00000000	0
0x00000008	0x00000000	0
0x00000009	0x00000000	0
0x0000000A	0x00000000	0
0x0000000B	0x00000000	0
0x0000000C	0x00000000	0
0x0000000D	0x00000000	0
0x0000000E	0x00000000	0
0x0000000F	0x444D4D59	1145916761
0x00000010	0x00000000	0
0x00000011	0x00000001	1
0x00000012	0x0000002A	42
0x00000013	0x0000002A	42
0x00000014	0x15000000	352321536
0x00000015	0x15000000	352321536
0x00000016	0x00000000	0
0x00000017	0x00000000	0
0x00000018	0x00000000	0
0x00000019	0x00000000	0
0x0000001A	0x00000000	0
0x0000001B	0x00000000	0
0x0000001C	0x00000000	0
0x0000001D	0x00000000	0
0x0000001E	0x00000000	0
0x0000001F	0x00000000	0

Device 6 DMA Bar:
Offset Content[offset]  Content[offset+0x4]  Content[offset+0x8] Content[offset+0xC]:
0x00000000	0x00000000	0x00000001	0x00000004	0x00000009
0x00000010	0x00000010	0x00000019	0x00000024	0x00000031
0x00000020	0x00000040	0x00000051	0x00000064	0x00000079
0x00000030	0x00000090	0x000000A9	0x000000C4	0x000000E1
0x00000040	0x00000100	0x00000121	0x00000144	0x00000169
0x00000050	0x00000190	0x000001B9	0x000001E4	0x00000211
0x00000060	0x00000240	0x00000000	0x00000000	0x00000000
0x00000070	0x00000000	0x00000000	0x00000000	0x00000000
0x00000080	0x00000000	0x00000000	0x00000000	0x00000000
0x00000090	0x00000000	0x00000000	0x00000000	0x00000000
0x000000A0	0x00000000	0x00000000	0x00000000	0x00000000
0x000000B0	0x00000000	0x00000000	0x00000000	0x00000000
0x000000C0	0x00000000	0x00000000	0x00000000	0x00000000
0x000000D0	0x00000000	0x00000000	0x00000000	0x00000000
...
```
It is possible to dump the register contents of each device by calling `cat /proc/mtcadummy`

## Behavior control

In addition to the special registers mentioned above, it is possible to control whether or not certain operations will produce an error by writing to /proc/mtcadummy.

Eg. `echo open:1 >/proc/mtcadummy` will cause all following attempts to open a device to fail. Calling `echo "open:0 spi:1" >/proc/mtcadummy` will then enable opening but force any SPI request to fail.
It is possible to send up to 255 characters to the device. If options are sent multiple times, the last option sent will win.

### Available options

Option | Description
-------|------------
open   | If set to 1, open() calls on the device will fail
read   | If set to 1, read() calls on the device will fail
write  | If set to 1, write() calls on the device will fail
spi    | If set to 1, the SPI simulation will fail.
