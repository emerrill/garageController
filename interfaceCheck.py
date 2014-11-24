#!/usr/bin/python

import shelve, time, sys, subprocess, os

RESTART_TIMEOUT = 21600 # 6 Hours.
HOSTNAME = "garage.merrilldigital.com"

# Check if we are not in repair mode and just return 255 for reboot.
if len(sys.argv) > 1:
    # TODO, actually try interface repair?
    line = sys.argv[1]
    if line is not "test":
        sys.exit(255)


def close_and_exit(code):
    shelf.close()
    sys.exit(code)

def check_garage_running():
    try:
        ps = subprocess.Popen(['ps', 'xa'], stdout=subprocess.PIPE)
        grep = subprocess.Popen(['grep', 'garage.py'], stdin=ps.stdout, stdout=subprocess.PIPE)
        output = subprocess.check_output(['grep', '-v', 'grep'], stdin=grep.stdout)
        if output:
            return 1
    except:
        return 0



shelf = shelve.open('/tmp/networkcheck')

if 'firsterror' not in shelf:
    shelf["firsterror"] = 0

    
response = os.system("ping -c 1 -t2 " + HOSTNAME + " > /dev/null 2>&1")

if response != 0:
    now = time.time()
    # First time we have errored. Set and return good.
    if shelf["firsterror"] == 0:
        shelf["firsterror"] = now
        close_and_exit(0)
    
    diff = now - shelf["firsterror"]
    if diff >= RESTART_TIMEOUT:
        # We have hit our timeout.
        if check_garage_running():
            # The garage door is moving. Ignore for now.
            close_and_exit(0)
        # Reboot.
        shelf["firsterror"] = 0
        close_and_exit(255)
else:
    # Clear the error.
    if shelf["firsterror"] != 0:
        shelf["firsterror"] = 0
    close_and_exit(0)

close_and_exit(0)

