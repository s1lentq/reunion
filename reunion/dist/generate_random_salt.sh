#!/bin/sh

_fail () {
    echo $'Could not generate salt automatically.\nPlease enter a 16-64 length random alphanumerical string to "SteamIdHashSalt=" option in reunion.cfg.'
    return 1
}

_success() {
    if grep -e ^"SteamIdHashSalt\s*=" reunion.cfg 2>&1 >/dev/null; then
        sed -i "s/SteamIdHashSalt\s*=.*/SteamIdHashSalt = $SALT/g" reunion.cfg 2>&1 >/dev/null
    else
        echo "SteamIdHashSalt = $SALT" >> reunion.cfg
    fi
    if ! grep ^"SteamIdHashSalt = $SALT" reunion.cfg 2>&1 >/dev/null; then
        echo [fail]
        _fail
        return 1
    fi
    echo [ok]
    echo "Done. \"SteamIdHashSalt = $SALT\" generated successfully and written to reunion.cfg."
    return 0
}

SALT=

if [ ! -f reunion.cfg ]; then
    echo Error: reunion.cfg does not exist in the current directory!
    exit 1
fi

echo -n "Testing for grep ... "
if ! command -v grep 2>&1 >/dev/null; then
    echo [fail]
    _fail
    exit 1
fi
echo [ok]

echo -n "Testing for wc ... "
if ! command -v wc 2>&1 >/dev/null; then
    echo [fail]
    _fail
    exit 1
fi
echo [ok]

echo -n "Testing for sed ... "
if ! command -v sed 2>&1 >/dev/null; then
    echo [fail]
    _fail
    exit 1
fi
echo [ok]

echo -n "Testing for openssl ... "
if command -v openssl 2>&1 >/dev/null; then
    echo [ok]
    echo -n "Generating salt ... "
    SALT="$(openssl rand -hex 16)"
    if [ "$(echo "$SALT" | wc -c)" -gt 31 ]; then
        _success
        exit $?
    else
        echo [fail]
    fi
else
    echo [not found]
fi

echo -n "Testing for /dev/urandom ... "
if [ ! -e /dev/urandom ]; then
    echo [fail]
    _fail
    exit 1
fi
echo [ok]

echo -n "Testing for hexdump ... "
if command -v hexdump 2>&1 >/dev/null; then
    echo [ok]
    echo -n "Generating salt ... "
    SALT="$(hexdump -vn16 -e'4/4 "%08x"' /dev/urandom)"
    if [ "$(echo "$SALT" | wc -c)" -gt 31 ]; then
        _success
        exit $?
    else
        echo [fail]
    fi
else
    echo [not found]
fi

echo -n "Testing for xxd ... "
if command -v xxd 2>&1 >/dev/null; then
    echo [ok]
    echo -n "Generating salt ... "
    SALT="$(xxd -l 16 -p /dev/urandom)"
    if [ "$(echo "$SALT" | wc -c)" -gt 31 ]; then
        _success
        exit $?
    else
        echo [fail]
    fi
else
    echo [not found]
fi

echo -n "Testing for tr ... "
if command -v tr 2>&1 >/dev/null; then
    echo [ok]
    echo -n "Generating salt ... "
    SALT="$(tr -dc 'a-f0-9' < /dev/urandom | head -c32)"
    if [ "$(echo "$SALT" | wc -c)" -gt 31 ]; then
        _success
        exit $?
    else
        echo [fail]
    fi
else
    echo [not found]
fi
_fail
exit 1

