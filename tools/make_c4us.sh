#!/usr/bin/bash

# example usage: OLDOBJVERSION=4,9,10,10 OBJVERSION=4.9.10.10 VERSION=342 C4GROUP=~/clonk/c4group ./make_c4us.sh

set -e

if [ -z "$C4GROUP" ]; then
	echo "Please define the environment variable C4GROUP to point to your c4group binary" >&2
	exit 1
fi

if [ -z "$VERSION" ]; then
	echo "Please define the environment variable VERSION and tell what version this will be (e.g. 342)" >&2
	exit 1
fi

if [ -z "$OLDOBJVERSION" ]; then
	echo "Please define the environment variable OLDOBJVERSION and tell what version the old definitions were (comma-separated, e.g. 4,9,10,10)" >&2
	exit 1
fi

if [ -z "$OBJVERSION" ]; then
	echo "Please define the environment variable OBJVERSION and tell what version the new definitions are (e.g. 4.9.10.10)" >&2
	exit 1
fi

SCRIPT_DIR="$(realpath $(dirname $0))"

TARGET_DIR="$(realpath .)"

TAG="v$VERSION"

if [ -z "$GET_win32" ]; then
	GET_win32="curl -L -o /tmp/LegacyClonk.zip https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Windows-x86.zip && unzip /tmp/LegacyClonk.zip && rm /tmp/LegacyClonk.zip"
fi

if [ -z "$GET_win64" ]; then
	GET_win64="curl -L -o /tmp/LegacyClonk.zip https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Windows-x64.zip && unzip /tmp/LegacyClonk.zip && rm /tmp/LegacyClonk.zip"
fi

if [ -z "$GET_linux" ]; then
	GET_linux="curl -L https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Linux-x86.tar.gz | tar xz"
fi

if [ -z "$GET_linux64" ]; then
	GET_linux64="curl -L https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Linux-x64.tar.gz | tar xz"
fi

if [ -z "$GET_mac" ]; then
	GET_mac="curl -L -o /tmp/LegacyClonk.zip https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Mac-x64.zip && unzip /tmp/LegacyClonk.zip && rm /tmp/LegacyClonk.zip"
fi

function win32() {
	mv clonk.exe "$1/Clonk.exe"
	mv clonk.pdb "$1/Clonk.pdb"
	mv *.pdb *.exe "$1"
}

function linux() {
	mv clonk c4group "$1"
}

function linux64() {
	linux "$@"
}

function win64() {
	win32 "$@"
}

function mac() {
	mv c4group "$1"
	mv clonk.app "$1/Clonk.app"
}

AUTOUPDATEFILE=$(cat <<EOF
Update
Name=$OBJVERSION
RequireVersion=$OLDOBJVERSION
EOF
)

for PLATFORM in "win32" "win64" "linux" "linux64" "mac"; do
	if [ -d "$TARGET_DIR/$PLATFORM" ]; then
		rm -r "$TARGET_DIR/$PLATFORM"
	fi
	mkdir -p "$TARGET_DIR/$PLATFORM"
	pushd "$TARGET_DIR/$PLATFORM"
	UPDATE=lc_${VERSION}_$PLATFORM.c4u
	mkdir $UPDATE
	cp "$TARGET_DIR/System.c4g.c4u" "$UPDATE"
	cp "$TARGET_DIR/Graphics.c4g.c4u" "$UPDATE"
	echo "$AUTOUPDATEFILE" > "$UPDATE/AutoUpdate.txt"
	CMD="GET_$PLATFORM"
	bash -c "${!CMD}"
	"$PLATFORM" "$UPDATE"
	"$C4GROUP" "$UPDATE" -p
	mv "$UPDATE" "$TARGET_DIR"
	popd
done
