#!/bin/sh

# if systemd is used and you need sound, login to $console prior https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=747882
# if you can't stand cursor bug, start this script from console, not another X https://bugs.launchpad.net/ubuntu/+source/plasma-workspace/+bug/1684240
# for many games you need running window manager, in that case run script which runs both wm and game via this script

display=1
console=1
tmpdir=$XDG_RUNTIME_DIR/nvidia
# see alternative packages at https://github.com/NixOS/nixpkgs/blob/master/nixos/modules/hardware/video/nvidia.nix#L14
package="linuxPackages.nvidia_x11"
busid=$($(nix-build --no-out-link '<nixpkgs>' -A pciutils)/bin/lspci | awk '/3D controller/{gsub(/\./,":",$1); print $1}')
kernel="$(uname -r)"

mkdir -p $tmpdir/modules
cat > $tmpdir/xorg.conf << EOF
Section "Files"
    ModulePath "$(nix-build --no-out-link '<nixpkgs>' -A xorg.xf86inputlibinput)/lib/xorg/modules/input"
    ModulePath "$(nix-build --no-out-link '<nixpkgs>' -A $package.bin)/lib/xorg/modules"
    ModulePath "$tmpdir/modules"
EndSection

Section "InputClass"
        Identifier "libinput pointer catchall"
        MatchIsPointer "on"
        MatchDevicePath "/dev/input/event*"
        Driver "libinput"
EndSection
Section "InputClass"
        Identifier "libinput keyboard catchall"
        MatchIsKeyboard "on"
        MatchDevicePath "/dev/input/event*"
        Driver "libinput"
EndSection
Section "InputClass"
        Identifier "libinput touchpad catchall"
        MatchIsTouchpad "on"
        MatchDevicePath "/dev/input/event*"
        Driver "libinput"
        Option "Tapping" "on"
        Option "DisableWhileTyping" "off"
        Option "MiddleEmulation" "on"
EndSection

Section "Module"
    Load "modesetting"
EndSection

Section "Device"
    Identifier "nvidia"
    Driver "nvidia"
    BusID "$busid"
    Option "AllowEmptyInitialConfiguration"
EndSection
EOF

red='\033[0;31m'
if [ ! -d $tmpdir ]; then
  echo -e "$red Given tmpdir doesn't exist"
  exit 1
fi
if [ "$1" == "" ]; then
  echo -e "$red Give path to game as argument"
  exit 1
fi
if [ "$busid" == "" ]; then
  echo -e "$red No Optimus card found"
  exit 1
fi
# module nvidia_drm is in use
if [ "$(fgconsole)" == "$console" ]; then
  echo -e "$red You need to run $0 from another console"
  exit 1
fi
if [ -z "$XAUTHORITY" ]; then
  export XAUTHORITY=$HOME/.Xauthority
fi

cat > $tmpdir/session << EOF
#!/bin/sh
$(nix-build --no-out-link '<nixpkgs>' -A xorg.xrandr)/bin/xrandr --setprovideroutputsource modesetting NVIDIA-0
$(nix-build --no-out-link '<nixpkgs>' -A xorg.xrandr)/bin/xrandr --auto
export LD_LIBRARY_PATH="$(nix-build --no-out-link -E '
  with import <nixpkgs> {};

  buildEnv {
    name = "nvidia-libs";
    paths = [
      libglvnd
      ('$package'.override {
        libsOnly = true;
        kernel = null;
      })
    ];
  }
')/lib`
  `:$(nix-build --no-out-link -E '
    with import <nixpkgs> {};

    buildEnv {
      name = "nvidia-libs-32";
      paths = with pkgsi686Linux; [
        libglvnd
        ('$package'.override {
          libsOnly = true;
          kernel = null;
        })
      ];
  }')/lib"
#export PATH="\$(nix-build --no-out-link -E '
#  with import <nixpkgs> {};
#
#  buildEnv {
#    name = "nvidia-bin";
#    paths = [ '$package'.settings '$package'.bin ];
#  }
#')/bin:\$PATH"
export VK_ICD_FILENAMES="$(nix-build --no-out-link -E '
  with import <nixpkgs> {};

  '$package'.override {
    libsOnly = true;
    kernel = null;
  }
')/share/vulkan/icd.d/nvidia.json";
export OCL_ICD_VENDORS="$(nix-build --no-out-link -E '
  with import <nixpkgs> {};

  '$package'.override {
    libsOnly = true;
    kernel = null;
  }
')/etc/OpenCL/vendors/nvidia.icd";
# https://bugzilla.gnome.org/show_bug.cgi?id=774775
export GDK_BACKEND=x11
$@
EOF
chmod +x $tmpdir/session

# switch user
cat > $tmpdir/wrapper << EOF
#!/bin/sh
sudo chown $USER $XAUTHORITY
sudo -u $USER $tmpdir/session
EOF
chmod +x $tmpdir/wrapper

# dixRegisterPrivateKey: Assertion `!global_keys[type].created' failed
cat > $tmpdir/X << EOF
#!/bin/sh
LD_LIBRARY_PATH="$(nix-build --no-out-link '<nixpkgs>' -A libglvnd)/lib"
exec $(nix-build --no-out-link '<nixpkgs>' -A xorg.xorgserver)/bin/X "\$@"
EOF
chmod +x $tmpdir/X

# https://bugs.freedesktop.org/show_bug.cgi?id=94577
ln -sf $(nix-build --no-out-link '<nixpkgs>' -A xorg.xorgserver)/lib/xorg/modules/* $tmpdir/modules
rm $tmpdir/modules/libglamoregl.so

if [ -e /etc/nixos ]; then
  # complicated to always check non-nixos rules on nixos
  bbswitch="$(nix-build --no-out-link -E '
    with import <nixpkgs/nixos> {};

    let
      custom = config.boot.kernelPackages;
      default = pkgs.linuxPackages;
    in default.bbswitch.override {
      kernel = default.kernel // {
        modDirVersion = "'$kernel'";
        inherit (custom.kernel) dev;
      };
    }
  ')"
  nvidia="$(nix-build --no-out-link -E '
    with import <nixpkgs/nixos> {};

    let
      custom = config.boot.kernelPackages;
    in (pkgs.'$package'.overrideAttrs (oldAttrs: rec {
      kernelVersion = "'$kernel'";
      kernel = custom.kernel.dev;
    })).bin
  ')"
else
  bbswitch="$(nix-build --no-out-link -E '
    with import <nixpkgs> {};

    let
      default = linuxPackages;
    in default.bbswitch.override {
      kernel = default.kernel // {
        modDirVersion = "'$kernel'";
        dev = "";
      };
    }
  ')"
  # needed for arch
  sudo ln -s /lib/modules/$kernel/build /lib/modules/$kernel/source
  kernel_compile_h=/lib/modules/$kernel/source/include/generated/compile.h
  if [ ! -f $kernel_compile_h ]; then
    echo -e "$red Install kernel headers package"
    exit 1
  fi
  kernel_cc_string=`cat ${kernel_compile_h} | grep LINUX_COMPILER | cut -f 2 -d '"'`
  kernel_cc_version=`echo ${kernel_cc_string} | grep -o '[0-9]\+\.[0-9]\+' | head -n 1`
  kernel_cc_major=`echo ${kernel_cc_version} | cut -d '.' -f 1`
  nvidia="$(nix-build --no-out-link -E '
    with import <nixpkgs> {};

    (('$package'.override { stdenv = overrideCC stdenv gcc'$kernel_cc_major'; }).overrideAttrs (oldAttrs: rec {
      kernelVersion = "'$kernel'";
      kernel = "";
      NIX_ENFORCE_PURITY = 0;
    })).bin
  ')"
fi

sudo insmod $bbswitch/lib/modules/$kernel/misc/bbswitch.ko
sudo tee /proc/acpi/bbswitch <<<ON

sudo modprobe ipmi_devintf
for m in nvidia nvidia-modeset nvidia-drm nvidia-uvm
do
  sudo insmod $nvidia/lib/modules/$kernel/misc/$m.ko
done

xinit=$(nix-build --no-out-link '<nixpkgs>' -A xorg.xinit)/bin
# xinit is unsecure
sudo PATH=$xinit:$PATH $xinit/startx $tmpdir/wrapper -- $tmpdir/X :$display -config $tmpdir/xorg.conf -logfile /var/log/X.$display.log vt$console
sudo chown $USER $XAUTHORITY

for m in nvidia_uvm nvidia_drm nvidia_modeset nvidia
do
  sudo rmmod $m
done
sudo tee /proc/acpi/bbswitch <<<OFF
