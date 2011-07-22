version=$1

if [ -z $version ]; then
  echo "specify version";
  exit;
fi

make CONF=Release32
make CONF=Release64

./release-unix.sh indigo-deco-$version-linux32 dist/Release32/GNU-Linux-x86/indigo-deco
./release-unix.sh indigo-deco-$version-linux64 dist/Release64/GNU-Linux-x86/indigo-deco
