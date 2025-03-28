#
# Include the robot settings
#
IMAGE_INSTALL += "cryptsetup"
IMAGE_INSTALL += "libatomic"
IMAGE_INSTALL += "libdevmapper"
IMAGE_INSTALL += "libsamplerate0"
IMAGE_INSTALL += "libsndfile1"
IMAGE_INSTALL += "lvm2"
IMAGE_INSTALL += "rng-tools"
IMAGE_INSTALL += "alsa-utils"

IMAGE_INSTALL += "syslog-ng"
IMAGE_INSTALL += "logrotate"
IMAGE_INSTALL += "kernel-module-ramoops"
IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','avahi', 'avahi-daemon', '', d)}"
