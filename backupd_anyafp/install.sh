#!/bin/bash
BASE=`dirname $0`

function do_stuff
{
    # Just in case, ya know..
    defaults write com.apple.systempreferences TMShowUnsupportedNetworkVolumes 1

    # Diff could have worked too..
    echo "(for our next trick, we need sudo)"
    test -f /System/Library/LaunchDaemons/com.apple.backupd.plist.bak || \
        sudo cp /System/Library/LaunchDaemons/com.apple.backupd.plist /System/Library/LaunchDaemons/com.apple.backupd.plist.bak
    sudo cp "${BASE}/com.apple.backupd.plist" /System/Library/LaunchDaemons/com.apple.backupd.plist
    sudo mkdir -p /usr/local/lib/
    sudo cp "${BASE}/libbackupd_anyfs.dylib" /usr/local/lib/libbackupd_anyfs.dylib

    sudo launchctl unload /System/Library/LaunchDaemons/com.apple.backupd.plist
    sudo launchctl load /System/Library/LaunchDaemons/com.apple.backupd.plist
    sudo killall backupd 2&>1 >/dev/null

    echo 
    exit 0
}

function check_my_stuff
{
    test -f "${BASE}/libbackupd_anyfs.dylib" -a -f "${BASE}/com.apple.backupd.plist"
}

check_my_stuff || { echo 'Seem to be missing a few nuts and bolts, ABORT, ABORT!!' ; exit 9001; }

echo -e "\n\nFaking AFP support; to unfake put back /System/Library/LaunchDaemons/com.apple.backupd.plist.bak"
echo -e "\n\n"'Type "My Data Is Doomed!" in all lowercase to proceed'
read text

test "$text" == 'my data is doomed' && { echo 'Forgot the exclamation mark!' ; exit 123; } 

if [ "$text" != 'my data is doomed!' ]; 
then 
	echo 'Smart choice!';
    sleep 2
    echo "Just kidding, you're chicken!";
    exit 1;
else
	do_stuff;
fi
