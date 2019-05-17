#!/bin/bash

# usage: installdmg https://example.com/path/to/pkg.dmg
function installdmg {
    set -x
    set -e
    tempd=$(mktemp -d)
    cp $1 $tempd/pkg.dmg
    listing=$(sudo hdiutil attach $tempd/pkg.dmg | grep Volumes)
    volume=$(echo "$listing" | cut -f 3)
    if [ -e "$volume"/*.app ]; then
      sudo cp -rf "$volume"/*.app /Applications
    elif [ -e "$volume"/*.pkg ]; then
      package=$(ls -1 "$volume" | grep .pkg | head -1)
      sudo installer -pkg "$volume"/"$package" -target /
    fi
    rm -rf $tempd
    sudo hdiutil detach "$(echo "$listing" | cut -f 3)"
    set +x
    set +e
	return 0
}

installdmg $1
