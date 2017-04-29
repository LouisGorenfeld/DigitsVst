# Digits VST

http://www.extentofthejam.com/

## license

Please read License.txt! :) Digits is licensed under the GPL. 

## building

### prerequisites

You'll need the Steinberg VST SDK which can be obtained from https://www.steinberg.net/en/company/developers.html.

### linux

    mkdir sdks
    ln -s /path/to/VST_SDK/VST2_SDK sdks/vstsdk2.4
    cd linux
    make -f Digits.make
