#!/usr/bin/env nix-shell
#!nix-shell -i bash -p xorg.xinit linuxPackages.bbswitch xorg.xorgserver xorg.xrandr

# if systemd used and you need sound, login to $console prior https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=747882
# if you can't stand cursor bug, start this script from console, not another X https://bugs.launchpad.net/ubuntu/+source/plasma-workspace/+bug/1684240

display=1
console=1
tmpdir=$XDG_RUNTIME_DIR/nvidia
# see alternative packages at https://github.com/NixOS/nixpkgs/blob/master/nixos/modules/hardware/video/nvidia.nix#L14
package="linuxPackages.nvidia_x11"

mkdir -p $tmpdir/modules
if [ -z "$XAUTHORITY" ]; then
  export XAUTHORITY=$HOME/.Xauthority
fi
# module nvidia_drm is in use
if [ "$(fgconsole)" == "$console" ]; then
  echo "You need to run $0 from another console"
  exit 1
fi

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
    BusID "PCI:1:0:0"
    Option "AllowEmptyInitialConfiguration"
EndSection
EOF

cat > $tmpdir/session << EOF
#!/bin/sh
xrandr --setprovideroutputsource modesetting NVIDIA-0
xrandr --auto
export LD_LIBRARY_PATH="$(nix-build --no-out-link -E 'with import <nixpkgs> {}; buildEnv { name = "nvidia-libs"; paths = [ libglvnd ('$package'.override { libsOnly = true; kernel = null; }) ]; }')/lib`
  `:$(nix-build --no-out-link -E 'with import <nixpkgs> {}; buildEnv { name = "nvidia-libs-32"; paths = with pkgsi686Linux; [ libglvnd ('$package'.override { libsOnly = true; kernel = null; }) ]; }')/lib"
#export PATH="\$(nix-build --no-out-link -E 'with import <nixpkgs> {}; buildEnv { name = "nvidia-bin"; paths = [ '$package'.settings '$package'.bin ]; }')/bin:$PATH"
export VK_ICD_FILENAMES="$(nix-build --no-out-link -E 'with import <nixpkgs> {}; '$package'.override { libsOnly = true; kernel = null; }')/share/vulkan/icd.d/nvidia.json";
export OCL_ICD_VENDORS="$(nix-build --no-out-link -E 'with import <nixpkgs> {}; '$package'.override { libsOnly = true; kernel = null; }')/etc/OpenCL/vendors/nvidia.icd";
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
exec X "\$@"
EOF
chmod +x $tmpdir/X

# https://bugs.freedesktop.org/show_bug.cgi?id=94577
ln -sf $(nix-build --no-out-link '<nixpkgs>' -A xorg.xorgserver)/lib/xorg/modules/* $tmpdir/modules
rm $tmpdir/modules/libglamoregl.so

sudo modprobe bbswitch
sudo tee /proc/acpi/bbswitch <<<ON
for m in nvidia nvidia_modeset nvidia_drm nvidia_uvm
do
  sudo modprobe $m
done

# xinit is unsecure
sudo startx $tmpdir/wrapper -- $tmpdir/X :$display -config $tmpdir/xorg.conf -logfile /var/log/X.$display.log vt$console
sudo chown $USER $XAUTHORITY

for m in nvidia_uvm nvidia_drm nvidia_modeset nvidia
do
  sudo rmmod $m
done
sudo tee /proc/acpi/bbswitch <<<OFF
