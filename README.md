# mct: a simple "container" runtime

The mct runtime is a very simple static-binary portable "container" runtime designed to be used for quick sandboxing of root image builds and experimentation.

It's not a replacement for docker, runc, crun, podman, etc - it is merely a chroot + namespacing that allows non-privileged users to spawn things that vaguely look like containers.

Further more, this is most decidedly *not* more secure than the battle-hardened solutions - DO NOT use it as such - YOU HAVE BEEN WARNED.

## Configuration

mct reads the first line of /etc/setuid to determine what uids to map into a container, and then calls newuidmap on a child process.

Have the first line of /etc/setuid for your user configured (LXC has some examples) and mct should just work.

## Usage

```console
$ # Create a holding directory for our "container" that is world-writable
$ mkdir -p alpine
$ chmod 777 alpine
$ # We create a root filesystem at alpine/root owned by the container "root"
$ mct -R / - mkdir alpine/root
$ mct -R / - tar -xzvf alpine-minirootfs-3.15.0-x86_64.tar.gz -C alpine/root
$ # Invoke a shell in the container
$ mct -R alpine/root - /bin/sh
/ # whoami
root
/ # exit
# Cleanup after ourselves
$ mct -R / - rm -rf alpine/root
```