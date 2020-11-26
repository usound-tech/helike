# Features

Please Press _Enter_ key to end a current BIST command. Type _help_ to list the commands.

## 1 Show Status
```
HELIKE Terminal (v0.4.0) May 27 2020 12:05:05

/> bist
/bist/> status

DAC TWEETER:   ENABLED
DAC WOOFER:    ENABLED
WOOFER AMP R:  ENABLED
WOOFER AMP L:  ENABLED
SDCARD FS:     NOT MOUNTED

```

## 2 Test buttons
```
HELIKE Terminal (v0.4.0) May 27 2020 12:05:05

/> bist
/bist/> buttons
Joystick:       Dipswitches: 1234 Dipswitch cfg: 1234
```
## 3 Test audio
```
HELIKE Terminal (v0.4.0) May 27 2020 12:05:05

/> bist
/bist/> audio
\
```
## 4 Re-run BIST
_Takes about 5 seconds_
```
HELIKE Terminal (v0.4.0) May 27 2020 12:05:05

/> bist
/bist/> run_bist

DAC TWEETER:   ENABLED
DAC WOOFER:    ENABLED
WOOFER AMP R:  ENABLED
WOOFER AMP L:  ENABLED
SDCARD FS:     NOT MOUNTED

```

## 5 SD card test
```
HELIKE Terminal (v0.4.0) May 27 2020 12:05:05

/> bist
/bist/> status

DAC TWEETER:   ENABLED
DAC WOOFER:    ENABLED
WOOFER AMP R:  ENABLED
WOOFER AMP L:  ENABLED
SDCARD FS:     NOT MOUNTED
/bist/> exit
/> system
/system/> ls

SDCARD FS:     NOT MOUNTED
```