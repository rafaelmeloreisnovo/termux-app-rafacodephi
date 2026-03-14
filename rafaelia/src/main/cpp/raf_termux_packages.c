/* raf_termux_packages.c
   IDs deterministicos de pacotes Termux (sem libc, gerado)
*/

#include "raf_termux_packages.h"

#define RAF_TERMUX_PKG_VERIFY_SEED 0xA5A55A5Au

#if RAF_TERMUX_PKG_NAME_PTR_ENABLE
#define RAF_PKG_INIT(ID, NLEN, FLG, VFY, NPTR) { (ID), (NLEN), (FLG), (VFY), (NPTR) }

static const char g_termux_pkg_names[] =
  "0verkill\0"
  "2048-c\0"
  "2ping\0"
  "6tunnel\0"
  "7zip\0"
  "8086tiny\0"
  "aalib\0"
  "aapt\0"
  "abduco\0"
  "abook\0"
  "abootimg\0"
  "abseil-cpp\0"
  "ack-grep\0"
  "acr\0"
  "ada\0"
  "adms\0"
  "aerc\0"
  "agate\0"
  "age\0"
  "agg\0"
  "aha\0"
  "aichat\0"
  "alass\0"
  "alembic\0"
  "algernon\0"
  "alpine\0"
  "alsa-lib\0"
  "alsa-plugins\0"
  "alsa-utils\0"
  "amber\0"
  "amfora\0"
  "android-tools\0"
  "anewer\0"
  "angband\0"
  "angle-android\0"
  "angle-grinder\0"
  "ani-cli\0"
  "ansifilter\0"
  "ant\0"
  "antibody\0"
  "antiword\0"
  "aosp-libs\0"
  "apache-orc\0"
  "apache2\0"
  "apkeep\0"
  "apksigner\0"
  "appstream\0"
  "apr\0"
  "apr-util\0"
  "apt\0"
  "apt-file\0"
  "apt-show-versions\0"
  "aptly\0"
  "argon2\0"
  "argp\0"
  "aria2\0"
  "arj\0"
  "arpack-ng\0"
  "artalk\0"
  "arturo\0"
  "ascii\0"
  "asciidoc\0"
  "asciidoctor\0"
  "asciinema\0"
  "asm-lsp\0"
  "aspell\0"
  "aspell-de\0"
  "aspell-en\0"
  "aspell-es\0"
  "aspell-fr\0"
  "assimp\0"
  "asymptote\0"
  "at\0"
  "at-spi2-core\0"
  "atomicparsley\0"
  "atomvm\0"
  "atool\0"
  "attr\0"
  "atuin\0"
  "aubio\0"
  "audiofile\0"
  "autoconf\0"
  "autoconf-archive\0"
  "autojump\0"
  "automake\0"
  "autossh\0"
  "aview\0"
  "avra\0"
  "await\0"
  "awscli\0"
  "axel\0"
  "b3sum\0"
  "babl\0"
  "bacula-fd\0"
  "barcode\0"
  "base16384\0"
  "bash\0"
  "bash-completion\0"
  "bastet\0"
  "bat\0"
  "bc\0"
  "bc-gh\0"
  "bcal\0"
  "bchunk\0"
  "bdsup2sub\0"
  "beanshell\0"
  "bear\0"
  "bed\0"
  "bftpd\0"
  "bgrep\0"
  "biboumi\0"
  "binaryen\0"
  "binutils\0"
  "bison\0"
  "bitcoin\0"
  "bitlbee\0"
  "bk\0"
  "blackbox\0"
  "blink\0"
  "blogc\0"
  "blueprint-compiler\0"
  "bmon\0"
  "boinc\0"
  "boinctui\0"
  "bombadillo\0"
  "boost\0"
  "bore\0"
  "borgbackup\0"
  "botan3\0"
  "boxes\0"
  "brainfuck\0"
  "brogue\0"
  "brook\0"
  "broot\0"
  "brotli\0"
  "bsd-finger\0"
  "bsd-games\0"
  "btfs2\0"
  "btrfs-progs\0"
  "buf\0"
  "build-essential\0"
  "busybox\0"
  "bvi\0"
  "byacc\0"
  "byobu\0"
  "c-ares\0"
  "c-toxcore\0"
  "ca-certificates\0"
  "cabal-install\0"
  "cabextract\0"
  "cabin\0"
  "cadaver\0"
  "caddy\0"
  "calc\0"
  "calcurse\0"
  "capnproto\0"
  "capstone\0"
  "carapace\0"
  "cargo-c\0"
  "cargo-leptos\0"
  "catdoc\0"
  "catgirl\0"
  "catimg\0"
  "cava\0"
  "cavez-of-phear\0"
  "cavif\0"
  "cboard\0"
  "cbonsai\0"
  "cc65\0"
  "ccache\0"
  "cccc\0"
  "ccextractor\0"
  "ccls\0"
  "ccrypt\0"
  "cfengine\0"
  "cfm\0"
  "cgal\0"
  "cgdb\0"
  "cgif\0"
  "chafa\0"
  "check\0"
  "chezmoi\0"
  "chicken\0"
  "chntpw\0"
  "choose\0"
  "chrony\0"
  "cicada\0"
  "ciso\0"
  "cjson\0"
  "ckermit\0"
  "clamav\0"
  "clblast\0"
  "clidle\0"
  "clifm\0"
  "clinfo\0"
  "clipp\0"
  "cloneit\0"
  "cloudflared\0"
  "clpeak\0"
  "clvk\0"
  "cmake\0"
  "cmark\0"
  "cmatrix\0"
  "cmocka\0"
  "cmus\0"
  "cmusfm\0"
  "codecrypt\0"
  "codon\0"
  "coinor-cbc\0"
  "coinor-clp\0"
  "cointop\0"
  "colm\0"
  "colordiff\0"
  "command-not-found\0"
  "composer\0"
  "console-bridge\0"
  "convertlit\0"
  "cookcli\0"
  "coreutils\0"
  "corgi\0"
  "corkscrew\0"
  "corrosion\0"
  "cowsay\0"
  "cpio\0"
  "cppcheck\0"
  "cppi\0"
  "cppunit\0"
  "cpufetch\0"
  "cpulimit\0"
  "crawl\0"
  "croc\0"
  "cronie\0"
  "crowbook\0"
  "crunch\0"
  "cryptopp\0"
  "crystal\0"
  "cscope\0"
  "csh\0"
  "csol\0"
  "csview\0"
  "ctags\0"
  "ctypes-sh\0"
  "cuetools\0"
  "cups\0"
  "cups-pdf\0"
  "curlie\0"
  "curseofwar\0"
  "cvs\0"
  "d8\0"
  "daemonize\0"
  "dar\0"
  "darkhttpd\0"
  "dart\0"
  "dasel\0"
  "dash\0"
  "dasm\0"
  "datamash\0"
  "dateutils\0"
  "dbus\0"
  "dbus-python\0"
  "dcmtk\0"
  "dcraw\0"
  "ddrescue\0"
  "debianutils\0"
  "debootstrap\0"
  "delve\0"
  "deno\0"
  "desed\0"
  "deutex\0"
  "dex2jar\0"
  "dialog\0"
  "dictd\0"
  "diff-so-fancy\0"
  "diffstat\0"
  "difftastic\0"
  "diffutils\0"
  "dirb\0"
  "direnv\0"
  "direvent\0"
  "discordo\0"
  "discount\0"
  "diskus\0"
  "distant\0"
  "distcc\0"
  "djvulibre\0"
  "dmagnetic\0"
  "dmtx-utils\0"
  "dnglab\0"
  "dnote\0"
  "dnote-server\0"
  "dns2tcp\0"
  "dnscontrol\0"
  "dnslookup\0"
  "dnsmap\0"
  "dnstop\0"
  "dnsutils\0"
  "docbook-xml\0"
  "docbook-xsl\0"
  "docopt\0"
  "doctest\0"
  "doge\0"
  "dopewars\0"
  "dos2unix\0"
  "dosfstools\0"
  "dotconf\0"
  "dotnet-host\0"
  "dotnet10.0\0"
  "dotnet8.0\0"
  "dotnet9.0\0"
  "double-conversion\0"
  "doxygen\0"
  "dpkg\0"
  "draco\0"
  "dropbear\0"
  "dtach\0"
  "dtc\0"
  "dte\0"
  "dua\0"
  "duc\0"
  "duf\0"
  "dufs\0"
  "dust\0"
  "dvdauthor\0"
  "dvtm\0"
  "dwarves\0"
  "dx\0"
  "e2fsprogs\0"
  "e2tools\0"
  "ebook-tools\0"
  "ecj\0"
  "ecl\0"
  "ed\0"
  "edbrowse\0"
  "editorconfig-core-c\0"
  "eigen\0"
  "eja\0"
  "electric-fence\0"
  "electrum\0"
  "elinks\0"
  "elixir\0"
  "eltclsh\0"
  "elvish\0"
  "emacs\0"
  "emmylua-ls\0"
  "emscripten\0"
  "enblend\0"
  "enchant\0"
  "enscript\0"
  "entr\0"
  "erlang\0"
  "esbuild\0"
  "espeak\0"
  "et\0"
  "etsh\0"
  "exercism\0"
  "exfatprogs\0"
  "exhale\0"
  "exiftool\0"
  "exiv2\0"
  "expect\0"
  "eza\0"
  "fact++\0"
  "fakeroot\0"
  "fasd\0"
  "fastfetch\0"
  "fastmod\0"
  "fatsort\0"
  "faust\0"
  "fclones\0"
  "fcp\0"
  "fd\0"
  "fdkaac\0"
  "fdm\0"
  "fdroidcl\0"
  "fdupes\0"
  "fennel\0"
  "feroxbuster\0"
  "fetchmail\0"
  "fff\0"
  "ffmpeg\0"
  "ffmpegthumbnailer\0"
  "ffsend\0"
  "fftw\0"
  "figlet\0"
  "file\0"
  "finch\0"
  "findomain\0"
  "findutils\0"
  "fish\0"
  "flang\0"
  "flatbuffers\0"
  "flex\0"
  "fluidsynth\0"
  "flyctl\0"
  "fm\0"
  "fmt\0"
  "fontconfig\0"
  "forgejo\0"
  "fortune\0"
  "fossil\0"
  "freecolor\0"
  "freeimage\0"
  "freetype\0"
  "frei0r-plugins\0"
  "fresh-editor\0"
  "fribidi\0"
  "frobtads\0"
  "frotz\0"
  "frp\0"
  "fselect\0"
  "fsmon\0"
  "fwknop\0"
  "fx\0"
  "fzf\0"
  "fzy\0"
  "game-music-emu\0"
  "gap\0"
  "gatling\0"
  "gauche\0"
  "gawk\0"
  "gbt\0"
  "gcab\0"
  "gcal\0"
  "gdal\0"
  "gdb\0"
  "gdbm\0"
  "gdk-pixbuf\0"
  "gdrive-downloader\0"
  "gdu\0"
  "geckodriver\0"
  "gecode\0"
  "gegl\0"
  "gengetopt\0"
  "geographiclib\0"
  "geoip2-database\0"
  "germanium\0"
  "getconf\0"
  "geth\0"
  "gettext\0"
  "gexiv2\0"
  "gflags\0"
  "gforth\0"
  "gh\0"
  "ghc\0"
  "ghostscript\0"
  "giflib\0"
  "gifsicle\0"
  "gifski\0"
  "git\0"
  "git-credential-manager\0"
  "git-crypt\0"
  "git-delta\0"
  "git-extras\0"
  "git-lfs\0"
  "git-sizer\0"
  "git-town\0"
  "gitea\0"
  "gitflow-avh\0"
  "gitoxide\0"
  "gitui\0"
  "gkermit\0"
  "glab-cli\0"
  "gleam\0"
  "glib\0"
  "glib-networking\0"
  "glibc-repo\0"
  "glm\0"
  "global\0"
  "glow\0"
  "glpk\0"
  "glslang\0"
  "gluelang\0"
  "glulxe\0"
  "gmic\0"
  "gn\0"
  "gnucap\0"
  "gnuchess\0"
  "gnucobol\0"
  "gnugo\0"
  "gnuit\0"
  "gnunet\0"
  "gnupg\0"
  "gnuplot\0"
  "gnurl\0"
  "gnushogi\0"
  "gnuski\0"
  "gnustep-make\0"
  "go-findimagedupes\0"
  "go-musicfox\0"
  "goaccess\0"
  "gobang\0"
  "gobject-introspection\0"
  "gogs\0"
  "gojq\0"
  "golang\0"
  "gomp\0"
  "gomuks\0"
  "google-glog\0"
  "googletest\0"
  "goose\0"
  "gopass\0"
  "gopher\0"
  "gopls\0"
  "goresym\0"
  "gost\0"
  "gotify\0"
  "gotop\0"
  "gotorrent\0"
  "gotty\0"
  "gpac\0"
  "gperf\0"
  "gpgme\0"
  "gpgmepp\0"
  "gping\0"
  "gpsbabel\0"
  "gradle\0"
  "grafana\0"
  "grap\0"
  "graphene\0"
  "graphicsmagick\0"
  "graphviz\0"
  "greed\0"
  "grep\0"
  "grex\0"
  "groff\0"
  "gron\0"
  "groonga\0"
  "groovy\0"
  "grpcurl\0"
  "gsl\0"
  "gst-libav\0"
  "gst-plugins-bad\0"
  "gst-plugins-base\0"
  "gst-plugins-good\0"
  "gst-plugins-ugly\0"
  "gst-python\0"
  "gstreamer\0"
  "gtypist\0"
  "guile\0"
  "gum\0"
  "gumbo-parser\0"
  "gweather-locations\0"
  "gzip\0"
  "haproxy\0"
  "harfbuzz\0"
  "hash-slinger\0"
  "hashdeep\0"
  "hcl\0"
  "hcloud\0"
  "helix\0"
  "hello\0"
  "helm\0"
  "helm-ls\0"
  "help2man\0"
  "hexcurse\0"
  "hexedit\0"
  "hexer\0"
  "hexyl\0"
  "heyu\0"
  "hfsutils\0"
  "hidapi\0"
  "hilbish\0"
  "hledger\0"
  "hledger-ui\0"
  "hoedown\0"
  "hollywood\0"
  "hors\0"
  "hstr\0"
  "html-xml-utils\0"
  "html2text\0"
  "htop\0"
  "htslib\0"
  "httping\0"
  "httrack\0"
  "hub\0"
  "hugo\0"
  "hummin\0"
  "hunspell\0"
  "hunspell-en-us\0"
  "hunspell-fr\0"
  "hunspell-hu\0"
  "hunspell-nl\0"
  "hunspell-ru\0"
  "hut\0"
  "hwdata\0"
  "hydroxide\0"
  "hyperfine\0"
  "hz\0"
  "i2pd\0"
  "iat\0"
  "icecast\0"
  "ices\0"
  "icon-naming-utils\0"
  "icoutils\0"
  "id3lib\0"
  "id3v2\0"
  "imagemagick\0"
  "imath\0"
  "imlib2\0"
  "indent\0"
  "inetutils\0"
  "influxdb\0"
  "innoextract\0"
  "inotify-tools\0"
  "intltool\0"
  "inxi\0"
  "ipcalc\0"
  "iperf3\0"
  "ipmitool\0"
  "iproute2\0"
  "ipv6calc\0"
  "ipv6toolkit\0"
  "ircd-irc2\0"
  "ired\0"
  "irssi\0"
  "isync\0"
  "iverilog\0"
  "iwyu\0"
  "jack\0"
  "jack-example-tools\0"
  "jack2\0"
  "jackett\0"
  "jadx\0"
  "jbig2dec\0"
  "jbig2enc\0"
  "jcal\0"
  "jellyfin-server\0"
  "jfrog-cli\0"
  "jftui\0"
  "jhead\0"
  "jigdo\0"
  "jira-go\0"
  "jless\0"
  "jo\0"
  "joe\0"
  "jove\0"
  "jp2a\0"
  "jpegoptim\0"
  "jq\0"
  "jq-lsp\0"
  "jql\0"
  "json-c\0"
  "json-glib\0"
  "jsoncpp\0"
  "jump\0"
  "jupp\0"
  "just\0"
  "jython\0"
  "k2pdfopt\0"
  "k9s\0"
  "kainjow-mustache\0"
  "kakoune\0"
  "kakoune-lsp\0"
  "kbd\0"
  "keybase\0"
  "keychain\0"
  "kibi\0"
  "kiwix-tools\0"
  "knockd\0"
  "kona\0"
  "kotlin\0"
  "krb5\0"
  "kubecolor\0"
  "kubectl\0"
  "kubelogin\0"
  "kubo\0"
  "ladspa-sdk\0"
  "lastpass-cli\0"
  "lazygit\0"
  "lcal\0"
  "ldc\0"
  "ldd\0"
  "ldns\0"
  "ledger\0"
  "lego\0"
  "leptonica\0"
  "less\0"
  "lesspipe\0"
  "leveldb\0"
  "lexbor\0"
  "lexter\0"
  "lf\0"
  "lfortran\0"
  "lftp\0"
  "lgogdownloader\0"
  "lhasa\0"
  "liba52\0"
  "libacl\0"
  "libaml\0"
  "libandroid-complex-math\0"
  "libandroid-execinfo\0"
  "libandroid-glob\0"
  "libandroid-posix-semaphore\0"
  "libandroid-selinux\0"
  "libandroid-shmem\0"
  "libandroid-spawn\0"
  "libandroid-stub\0"
  "libandroid-support\0"
  "libandroid-sysv-semaphore\0"
  "libandroid-utimes\0"
  "libandroid-wordexp\0"
  "libao\0"
  "libaom\0"
  "libapt-pkg-perl\0"
  "libarchive\0"
  "libarrow-cpp\0"
  "libasio\0"
  "libass\0"
  "libassuan\0"
  "libatomic-ops\0"
  "libavif\0"
  "libbcprov-java\0"
  "libblosc\0"
  "libblosc2\0"
  "libbluray\0"
  "libbs2b\0"
  "libbsd\0"
  "libbullet\0"
  "libburn\0"
  "libbz2\0"
  "libc++\0"
  "libc++utilities\0"
  "libc-client\0"
  "libcaca\0"
  "libcairo\0"
  "libcairomm-1.0\0"
  "libcairomm-1.16\0"
  "libcap\0"
  "libcap-ng\0"
  "libccd\0"
  "libcddb\0"
  "libcdk\0"
  "libcec\0"
  "libceres-solver\0"
  "libchipmunk\0"
  "libchromaprint\0"
  "libclc\0"
  "libcln\0"
  "libcoap\0"
  "libcoinor-cgl\0"
  "libcoinor-osi\0"
  "libcoinor-utils\0"
  "libcommons-lang3-java\0"
  "libconfig\0"
  "libconfuse\0"
  "libcpufeatures\0"
  "libcroco\0"
  "libcrypt\0"
  "libcue\0"
  "libcunit\0"
  "libcurl\0"
  "libczmq\0"
  "libdaemon\0"
  "libdart\0"
  "libdav1d\0"
  "libdb\0"
  "libde265\0"
  "libdeflate\0"
  "libdevil\0"
  "libdispatch\0"
  "libdisplay-info\0"
  "libdjinterop\0"
  "libdmtx\0"
  "libdrm\0"
  "libduckdb\0"
  "libduktape\0"
  "libdvbcsa\0"
  "libdvbpsi\0"
  "libdvdnav\0"
  "libdvdread\0"
  "libebml\0"
  "libebur128\0"
  "libedit\0"
  "libelf\0"
  "libenet\0"
  "libepoxy\0"
  "libesqlite3\0"
  "libetonyek\0"
  "libev\0"
  "libevent\0"
  "libexif\0"
  "libexpat\0"
  "libfann\0"
  "libfcl\0"
  "libfdk-aac\0"
  "libffi\0"
  "libfixposix\0"
  "libflac\0"
  "libflann\0"
  "libforestdb\0"
  "libfreexl\0"
  "libftxui\0"
  "libfyaml\0"
  "libgc\0"
  "libgcrypt\0"
  "libgd\0"
  "libgedit-gfls\0"
  "libgee\0"
  "libgeos\0"
  "libgeotiff\0"
  "libgf2x\0"
  "libgfshare\0"
  "libgit2\0"
  "libglibmm-2.4\0"
  "libglibmm-2.68\0"
  "libglvnd\0"
  "libgmime\0"
  "libgmp\0"
  "libgnt\0"
  "libgnustep-base\0"
  "libgnutls\0"
  "libgpg-error\0"
  "libgraphite\0"
  "libgrpc\0"
  "libgsasl\0"
  "libgsf\0"
  "libgtop\0"
  "libgts\0"
  "libgxps\0"
  "libhangul\0"
  "libharu\0"
  "libhdf5\0"
  "libheif\0"
  "libhiredis\0"
  "libhtmlcxx\0"
  "libhyphen\0"
  "libical\0"
  "libice\0"
  "libiconv\0"
  "libicu\0"
  "libid3tag\0"
  "libidn\0"
  "libidn2\0"
  "libimagequant\0"
  "libimobiledevice\0"
  "libimobiledevice-glue\0"
  "libimtui\0"
  "libinih\0"
  "libiniparser\0"
  "libiodbc\0"
  "libisoburn\0"
  "libisofs\0"
  "libjanet\0"
  "libjansi\0"
  "libjansson\0"
  "libjasper\0"
  "libjpeg-turbo\0"
  "libjxl\0"
  "libkeyfinder\0"
  "libkiwix\0"
  "libknot\0"
  "libkokkos\0"
  "libksba\0"
  "liblangtag\0"
  "liblightning\0"
  "libliquid-dsp\0"
  "libllvm\0"
  "liblmdb\0"
  "liblo\0"
  "liblog4c\0"
  "liblog4cxx\0"
  "liblqr\0"
  "liblrdf\0"
  "liblz4\0"
  "liblzma\0"
  "liblzo\0"
  "libmaa\0"
  "libmad\0"
  "libmariadbcpp\0"
  "libmatio\0"
  "libmatroska\0"
  "libmaxminddb\0"
  "libmcrypt\0"
  "libmd\0"
  "libmdbx\0"
  "libmediainfo\0"
  "libmesode\0"
  "libmhash\0"
  "libmicrohttpd\0"
  "libminizip\0"
  "libminizip-ng\0"
  "libmnl\0"
  "libmodplug\0"
  "libmosquitto\0"
  "libmp3lame\0"
  "libmp3splt\0"
  "libmpc\0"
  "libmpdclient\0"
  "libmpeg2\0"
  "libmpfr\0"
  "libmpg123\0"
  "libmsgpack\0"
  "libmsgpack-cxx\0"
  "libmuparser\0"
  "libmypaint\0"
  "libnats-c\0"
  "libneon\0"
  "libnet\0"
  "libnettle\0"
  "libnewt\0"
  "libnfc\0"
  "libnfs\0"
  "libnftnl\0"
  "libnghttp2\0"
  "libnghttp3\0"
  "libngtcp2\0"
  "libnl\0"
  "libnova\0"
  "libnpth\0"
  "libnspr\0"
  "libnss\0"
  "libntl\0"
  "libobjc2\0"
  "libode\0"
  "libodfgen\0"
  "libogg\0"
  "liboggz\0"
  "libolm\0"
  "libopenblas\0"
  "libopencc\0"
  "libopencore-amr\0"
  "libopenfec\0"
  "libopenmpt\0"
  "libopus\0"
  "libopusenc\0"
  "liborc\0"
  "libosmium\0"
  "libosmpbf\0"
  "libotr\0"
  "libowfat\0"
  "libp11\0"
  "libp8-platform\0"
  "libpagmo\0"
  "libpangomm-1.4\0"
  "libpangomm-2.48\0"
  "libpano13\0"
  "libpaper\0"
  "libpcap\0"
  "libpcsclite\0"
  "libphysfs\0"
  "libpinyin\0"
  "libpipeline\0"
  "libpixman\0"
  "libplacebo\0"
  "libplist\0"
  "libpluto\0"
  "libpng\0"
  "libpngwriter\0"
  "libpoco\0"
  "libpopt\0"
  "libprotobuf\0"
  "libprotobuf-c\0"
  "libprotozero\0"
  "libpsl\0"
  "libpugixml\0"
  "libqrencode\0"
  "libraptor2\0"
  "libraqm\0"
  "librasqal\0"
  "librav1e\0"
  "libraw\0"
  "libre2\0"
  "libregexp-assemble-perl\0"
  "libresolv-wrapper\0"
  "libretls\0"
  "librevenge\0"
  "librime\0"
  "librinutils\0"
  "librnnoise\0"
  "librocksdb\0"
  "librsvg\0"
  "librsync\0"
  "librtmidi\0"
  "librttopo\0"
  "libsamplerate\0"
  "libsasl\0"
  "libsass\0"
  "libsearpc\0"
  "libseat\0"
  "libseccomp\0"
  "libsecp256k1\0"
  "libsecret\0"
  "libsfdo\0"
  "libshout\0"
  "libsigc++-2.0\0"
  "libsigc++-3.0\0"
  "libsignal-protocol-c\0"
  "libsigsegv\0"
  "libsixel\0"
  "libskiasharp\0"
  "libslirp\0"
  "libsm\0"
  "libsnappy\0"
  "libsndfile\0"
  "libsodium\0"
  "libsoldout\0"
  "libsophia\0"
  "libsoundtouch\0"
  "libsoup\0"
  "libsoup3\0"
  "libsoxr\0"
  "libspatialindex\0"
  "libspatialite\0"
  "libspdlog\0"
  "libspectre\0"
  "libspeex\0"
  "libspice-protocol\0"
  "libspice-server\0"
  "libsqlite\0"
  "libsrt\0"
  "libssh\0"
  "libssh2\0"
  "libstemmer\0"
  "libstrophe\0"
  "libt3config\0"
  "libt3highlight\0"
  "libt3key\0"
  "libt3widget\0"
  "libt3window\0"
  "libtalloc\0"
  "libtasn1\0"
  "libtatsu\0"
  "libtbb\0"
  "libtd\0"
  "libtdb\0"
  "libtermkey\0"
  "libtheora\0"
  "libthread-db\0"
  "libtiff\0"
  "libtiledb\0"
  "libtins\0"
  "libtinyxml\0"
  "libtinyxml2\0"
  "libtirpc\0"
  "libtllist\0"
  "libtomcrypt\0"
  "libtommath\0"
  "libtool\0"
  "libtorrent\0"
  "libtorrent-rasterbar\0"
  "libtpms\0"
  "libtranscript\0"
  "libtree-ldd\0"
  "libtsduck\0"
  "libtvision\0"
  "libtwolame\0"
  "libuber-h3\0"
  "libuchardet\0"
  "libucontext\0"
  "libudfread\0"
  "libunbound\0"
  "libunibilium\0"
  "libunistring\0"
  "libunqlite\0"
  "liburcu\0"
  "libusb\0"
  "libusbmuxd\0"
  "libusbredir\0"
  "libuv\0"
  "libv4l\0"
  "libvbisam\0"
  "libvidstab\0"
  "libvigra\0"
  "libvips\0"
  "libvisio\0"
  "libvmaf\0"
  "libvo-amrwbenc\0"
  "libvoikko\0"
  "libvorbis\0"
  "libvpx\0"
  "libvterm\0"
  "libvxl\0"
  "libwavpack\0"
  "libwayland\0"
  "libwayland-protocols\0"
  "libwebp\0"
  "libwebrtc-audio-processing\0"
  "libwebsockets\0"
  "libwolfssl\0"
  "libwpd\0"
  "libwpg\0"
  "libwps\0"
  "libwren\0"
  "libwslay\0"
  "libwv\0"
  "libx11\0"
  "libx264\0"
  "libx265\0"
  "libxapian\0"
  "libxau\0"
  "libxcb\0"
  "libxcfun\0"
  "libxcursor\0"
  "libxdg-basedir\0"
  "libxdmcp\0"
  "libxdrfile\0"
  "libxext\0"
  "libxfixes\0"
  "libxft\0"
  "libxi\0"
  "libxls\0"
  "libxlsxwriter\0"
  "libxml2\0"
  "libxmlb\0"
  "libxmlrpc\0"
  "libxrandr\0"
  "libxrender\0"
  "libxshmfence\0"
  "libxslt\0"
  "libxss\0"
  "libxt\0"
  "libxtst\0"
  "libxv\0"
  "libxxf86vm\0"
  "libyaml\0"
  "libyaml-cpp\0"
  "libzen\0"
  "libzim\0"
  "libzimg\0"
  "libzip\0"
  "libzita-convolver\0"
  "libzix\0"
  "libzmq\0"
  "libzopfli\0"
  "libzxing-cpp\0"
  "lighttpd\0"
  "lilv\0"
  "lilypond\0"
  "links\0"
  "lipl\0"
  "lit\0"
  "litespeedtest\0"
  "littlecms\0"
  "llama-cpp\0"
  "llbuild\0"
  "llvm-mingw-w64\0"
  "llvm-mingw-w64-tools\0"
  "lnav\0"
  "lnd\0"
  "locustdb\0"
  "logo-ls\0"
  "logrotate\0"
  "loksh\0"
  "lowdown\0"
  "lr\0"
  "lrzip\0"
  "lrzsz\0"
  "lsd\0"
  "lsix\0"
  "lsof\0"
  "ltrace\0"
  "lua-language-server\0"
  "lua-lgi\0"
  "lua-lpeg\0"
  "lua51\0"
  "lua52\0"
  "lua53\0"
  "lua54\0"
  "lua55\0"
  "luajit\0"
  "luajit-lgi\0"
  "luarocks\0"
  "luv\0"
  "luvi\0"
  "luvit\0"
  "lux\0"
  "lux-cli\0"
  "lv2\0"
  "lychee\0"
  "lynx\0"
  "lyrebird\0"
  "lzip\0"
  "lzlib\0"
  "lzop\0"
  "m4\0"
  "macchina\0"
  "magic-wormhole-rs\0"
  "mailsync\0"
  "mailutils\0"
  "make\0"
  "make-guile\0"
  "mandoc\0"
  "mangal\0"
  "manim\0"
  "manpages\0"
  "mapserver\0"
  "mariadb\0"
  "marisa\0"
  "markdown-flashcards\0"
  "marksman\0"
  "mathomatic\0"
  "matplotlib\0"
  "matterbridge\0"
  "matterircd\0"
  "matugen\0"
  "mautrix-whatsapp\0"
  "maven\0"
  "maxcso\0"
  "maxima\0"
  "mazter\0"
  "mbedtls\0"
  "mc\0"
  "mcfly\0"
  "mdbook\0"
  "mdbook-auto-gen-summary\0"
  "mdbook-cat-prep\0"
  "mdbook-epub\0"
  "mdbook-graphviz\0"
  "mdbook-katex\0"
  "mdbook-linkcheck\0"
  "mdbook-mermaid\0"
  "mdbook-open-on-gh\0"
  "mdbook-pikchr\0"
  "mdbook-plantuml\0"
  "mdbook-presentation-preprocessor\0"
  "mdbook-svgbob\0"
  "mdbook-svgbob2\0"
  "mdbook-tera\0"
  "mdbook-toc\0"
  "mdbtools\0"
  "mdds\0"
  "mdns-scan\0"
  "mdp\0"
  "media-types\0"
  "mediainfo\0"
  "mediamtx\0"
  "megacmd\0"
  "megatools\0"
  "memcached\0"
  "mercury\0"
  "mesa\0"
  "mfcuk\0"
  "mg\0"
  "micro\0"
  "microsocks\0"
  "miller\0"
  "mimetic\0"
  "minesweeper\0"
  "minicom\0"
  "minidlna\0"
  "miniflux\0"
  "minimodem\0"
  "minio\0"
  "miniserve\0"
  "minisign\0"
  "miniupnpc\0"
  "minizinc\0"
  "mkbootimg\0"
  "mkp224o\0"
  "mksh\0"
  "mktorrent\0"
  "mlocate\0"
  "mold\0"
  "monero\0"
  "monetdb\0"
  "mono\0"
  "monolith\0"
  "moon-buggy\0"
  "moor\0"
  "mop\0"
  "moreutils\0"
  "moria\0"
  "morse2ascii\0"
  "mosh\0"
  "most\0"
  "mp3cat\0"
  "mp3cat-go\0"
  "mp3gain\0"
  "mp3splt\0"
  "mp3wrap\0"
  "mpc\0"
  "mpd\0"
  "mpdscribble\0"
  "mplayer\0"
  "mpv\0"
  "mruby\0"
  "ms-gsl\0"
  "msedit\0"
  "msitools\0"
  "msmtp\0"
  "mtd-utils\0"
  "mtools\0"
  "mu\0"
  "muchsync\0"
  "mujs\0"
  "multitail\0"
  "mupdf\0"
  "music-file-organizer\0"
  "mutt\0"
  "myman\0"
  "mympd\0"
  "mypaint-brushes\0"
  "myrepos\0"
  "n-t-roff-sc\0"
  "n2n\0"
  "nala\0"
  "nali\0"
  "nano\0"
  "nasm\0"
  "natpmpc\0"
  "navi\0"
  "navidrome\0"
  "ncdc\0"
  "ncdu\0"
  "ncdu2\0"
  "ncftp\0"
  "ncmpcpp\0"
  "ncompress\0"
  "ncpamixer\0"
  "ncspot\0"
  "ncurses\0"
  "ndk-multilib\0"
  "ndk-sysroot\0"
  "ne\0"
  "nelua\0"
  "neocmakelsp\0"
  "neofetch\0"
  "neomutt\0"
  "neovim\0"
  "neovim-nightly\0"
  "nerdfix\0"
  "net-snmp\0"
  "net-tools\0"
  "netcat-openbsd\0"
  "netcdf-c\0"
  "nethack\0"
  "netpbm\0"
  "netsed\0"
  "netstandard-targeting-pack-2.1\0"
  "newsboat\0"
  "newsraft\0"
  "nginx\0"
  "ngircd\0"
  "ngspice\0"
  "nim\0"
  "ninja\0"
  "ninvaders\0"
  "nlohmann-json\0"
  "nlopt\0"
  "nmap\0"
  "nmh\0"
  "nmon\0"
  "nnn\0"
  "no-more-secrets\0"
  "nodejs\0"
  "nodejs-lts\0"
  "notcurses\0"
  "notmuch\0"
  "npm\0"
  "npush\0"
  "nsis\0"
  "nsnake\0"
  "nudoku\0"
  "nushell\0"
  "nyancat\0"
  "nzbget\0"
  "oathtool\0"
  "ocl-icd\0"
  "ocrad\0"
  "octave\0"
  "octomap\0"
  "odt2txt\0"
  "oh-my-posh\0"
  "oidn\0"
  "okc-agents\0"
  "ol\0"
  "oleo\0"
  "ollama\0"
  "oma\0"
  "onefetch\0"
  "onigmo\0"
  "oniguruma\0"
  "onnxruntime\0"
  "oorexx\0"
  "open-adventure\0"
  "openal-soft\0"
  "openbabel\0"
  "opencl-clhpp\0"
  "opencl-headers\0"
  "opencl-vendor-driver\0"
  "opencolorio\0"
  "openexr\0"
  "openfoam\0"
  "opengl\0"
  "openh264\0"
  "openjdk-17\0"
  "openjdk-21\0"
  "openjdk-25\0"
  "openjpeg\0"
  "openldap\0"
  "openlist\0"
  "openmpi\0"
  "openpgl\0"
  "opensc\0"
  "openscad\0"
  "openssh\0"
  "openssl\0"
  "opentimelineio\0"
  "openxr\0"
  "optipng\0"
  "opus-tools\0"
  "opusfile\0"
  "opustags\0"
  "orbiton\0"
  "osm2pgsql\0"
  "osmctools\0"
  "osmium-tool\0"
  "osslsigncode\0"
  "ossp-uuid\0"
  "ovmf\0"
  "oxlint\0"
  "p11-kit\0"
  "p7zip\0"
  "pacman\0"
  "pacman4console\0"
  "panda3d\0"
  "pandoc\0"
  "pango\0"
  "paperkey\0"
  "par2\0"
  "parallel\0"
  "pari\0"
  "parted\0"
  "paruz\0"
  "pass\0"
  "pass-otp\0"
  "passage\0"
  "passphrase2pgp\0"
  "pastebinit\0"
  "pastel\0"
  "patch\0"
  "patchelf\0"
  "pathpicker\0"
  "pb\0"
  "pcal\0"
  "pcaudiolib\0"
  "pcre\0"
  "pcre2\0"
  "pdf2svg\0"
  "pdfcpu\0"
  "pdfgrep\0"
  "pdftk\0"
  "peaclock\0"
  "peco\0"
  "perl\0"
  "perl-rename\0"
  "pet\0"
  "pforth\0"
  "pgroonga\0"
  "photon-rss\0"
  "php\0"
  "php-apcu\0"
  "php-imagick\0"
  "php-psr\0"
  "php-redis\0"
  "php-zephir-parser\0"
  "phpmyadmin\0"
  "pianobar\0"
  "pick\0"
  "picocom\0"
  "picolisp\0"
  "pigz\0"
  "pikiwidb\0"
  "pinentry\0"
  "pingme\0"
  "pipebuffer\0"
  "pipes.sh\0"
  "pipewire\0"
  "pkg-config\0"
  "pkgfile\0"
  "pkgtop\0"
  "plantuml\0"
  "play-audio\0"
  "plzip\0"
  "pngcrush\0"
  "pngquant\0"
  "pocketbase\0"
  "poke\0"
  "polipo\0"
  "polyml\0"
  "pomodoro-curses\0"
  "poppler\0"
  "poppler-data\0"
  "portaudio\0"
  "portmidi\0"
  "posixvala\0"
  "postgis\0"
  "postgresql\0"
  "potrace\0"
  "pounce\0"
  "povray\0"
  "predict\0"
  "privoxy\0"
  "procps\0"
  "procs\0"
  "procyon-decompiler\0"
  "profanity\0"
  "progress\0"
  "proj\0"
  "proot\0"
  "proot-distro\0"
  "proton-bridge\0"
  "prover9\0"
  "proxmark3\0"
  "proxychains-ng\0"
  "psmisc\0"
  "ptex\0"
  "ptunnel-ng\0"
  "pueue\0"
  "pulseaudio\0"
  "pup\0"
  "pure-ftpd\0"
  "pv\0"
  "pwgen\0"
  "pybind11\0"
  "pycairo\0"
  "pygobject\0"
  "pypy\0"
  "pypy3\0"
  "pyrefly\0"
  "pystring\0"
  "python\0"
  "python-apsw\0"
  "python-apt\0"
  "python-bcrypt\0"
  "python-brotli\0"
  "python-cmake\0"
  "python-contourpy\0"
  "python-cryptography\0"
  "python-greenlet\0"
  "python-grpcio\0"
  "python-lameenc\0"
  "python-libsass\0"
  "python-llvmlite\0"
  "python-lxml\0"
  "python-msgpack\0"
  "python-numpy\0"
  "python-pillow\0"
  "python-pip\0"
  "python-psutil\0"
  "python-pycryptodomex\0"
  "python-pynvim\0"
  "python-sabyenc3\0"
  "python-scipy\0"
  "python-skia-pathops\0"
  "python-tflite-runtime\0"
  "python-tldp\0"
  "python-torch\0"
  "python-torchaudio\0"
  "python-torchcodec\0"
  "python-torchvision\0"
  "python-xlib\0"
  "python-yt-dlp\0"
  "python2\0"
  "q-dns-client\0"
  "qalc\0"
  "qemu-system-x86-64-headless\0"
  "qhull\0"
  "qpdf\0"
  "qrsspig\0"
  "qrupdate-ng\0"
  "quick-lint-js\0"
  "quickjs-ng\0"
  "quilt\0"
  "rabbitmq-server\0"
  "racket\0"
  "radare2\0"
  "rage\0"
  "ragel\0"
  "railway-cli\0"
  "range-v3\0"
  "ranger\0"
  "rapidjson\0"
  "ratt\0"
  "ravencoin\0"
  "rbw\0"
  "rc\0"
  "rclone\0"
  "rcm\0"
  "rcs\0"
  "rdfind\0"
  "rdiff-backup\0"
  "rdircd\0"
  "rdrview\0"
  "re2c\0"
  "readline\0"
  "recode\0"
  "recoll\0"
  "recutils\0"
  "redir\0"
  "redis\0"
  "remind\0"
  "renameutils\0"
  "reptyr\0"
  "resolv-conf\0"
  "restic\0"
  "restic-server\0"
  "restish\0"
  "rgbds\0"
  "rhash\0"
  "rig\0"
  "rinetd\0"
  "rip2\0"
  "ripgrep\0"
  "ripgrep-all\0"
  "ripsecrets\0"
  "rirc\0"
  "rizin\0"
  "rlwrap\0"
  "rmpc\0"
  "rnr\0"
  "robin-map\0"
  "robotfindskitten\0"
  "root-repo\0"
  "rp-pppoe\0"
  "rpm\0"
  "rq\0"
  "rsgain\0"
  "rsnapshot\0"
  "rsync\0"
  "rtmpdump\0"
  "rtorrent\0"
  "rubberband\0"
  "rubiks-cube\0"
  "ruby\0"
  "ruff\0"
  "runit\0"
  "rush\0"
  "rust\0"
  "rust-analyzer\0"
  "rust-bindgen\0"
  "rustscan\0"
  "rxfetch\0"
  "sabnzbd\0"
  "samba\0"
  "samefile\0"
  "samurai\0"
  "sassc\0"
  "sbcl\0"
  "sc-im\0"
  "scala\0"
  "scc\0"
  "sccache\0"
  "scdoc\0"
  "screen\0"
  "screenfetch\0"
  "scrub\0"
  "scrypt\0"
  "sd\0"
  "sdcv\0"
  "seafile-client\0"
  "seanime\0"
  "seccure\0"
  "secure-delete\0"
  "sed\0"
  "sendme\0"
  "sendxmpp\0"
  "senpai\0"
  "sensible-utils\0"
  "serd\0"
  "serf\0"
  "sfeed\0"
  "sftpgo\0"
  "shaderc\0"
  "sharutils\0"
  "shc\0"
  "sheldon\0"
  "shell2http\0"
  "shellcheck\0"
  "shellharden\0"
  "shellinabox\0"
  "shfmt\0"
  "shiori\0"
  "shntool\0"
  "shtool\0"
  "signify\0"
  "silicon\0"
  "silversearcher-ag\0"
  "simde\0"
  "simdjson\0"
  "simh\0"
  "simulavr\0"
  "sing-box\0"
  "skate\0"
  "sl\0"
  "slang\0"
  "slashtime\0"
  "sleuthkit\0"
  "slides\0"
  "slugify\0"
  "smalltalk\0"
  "snapcast-server\0"
  "snmptt\0"
  "snowflake\0"
  "socat\0"
  "softether-vpn\0"
  "soju\0"
  "solidity\0"
  "sops\0"
  "sord\0"
  "sound-theme-freedesktop\0"
  "sox\0"
  "spatialite-tools\0"
  "speechd\0"
  "speedtest-go\0"
  "speexdsp\0"
  "spglib\0"
  "spidermonkey\0"
  "spiped\0"
  "spirv-headers\0"
  "spirv-llvm-translator\0"
  "spirv-tools\0"
  "sqlcipher\0"
  "squashfs-tools-ng\0"
  "squeezelite\0"
  "squid\0"
  "sratom\0"
  "srelay\0"
  "srt2vobsub\0"
  "ssdb\0"
  "ssdeep\0"
  "sse2neon\0"
  "sshpass\0"
  "sshping\0"
  "sslscan\0"
  "sssnake\0"
  "ssss\0"
  "stag\0"
  "starship\0"
  "stdman\0"
  "stdoutisatty\0"
  "steghide\0"
  "step-cli\0"
  "stfl\0"
  "stockfish\0"
  "stoken\0"
  "stone\0"
  "storj-uplink\0"
  "stow\0"
  "strace\0"
  "streamripper\0"
  "stunnel\0"
  "stuntman\0"
  "stylua\0"
  "subtitleripper\0"
  "subversion\0"
  "sudo\0"
  "suil\0"
  "suite3270\0"
  "suitesparse\0"
  "sun\0"
  "sundials\0"
  "supercollider\0"
  "surfraw\0"
  "svt-av1\0"
  "swaks\0"
  "swi-prolog\0"
  "swift\0"
  "swiftshader\0"
  "swig\0"
  "swtpm\0"
  "syncthing\0"
  "sysprop\0"
  "ta-lib\0"
  "taglib\0"
  "taplo\0"
  "tar\0"
  "task-spooler\0"
  "tasksh\0"
  "taskwarrior\0"
  "tcc\0"
  "tcl\0"
  "tcllib\0"
  "tcsh\0"
  "tdl\0"
  "tea\0"
  "tealdeer\0"
  "teckit\0"
  "tectonic\0"
  "telegram-bot-api\0"
  "teleport-tsh\0"
  "tere\0"
  "tergent\0"
  "termimage\0"
  "termplay\0"
  "termux-am\0"
  "termux-am-socket\0"
  "termux-api\0"
  "termux-apt-repo\0"
  "termux-auth\0"
  "termux-core\0"
  "termux-create-package\0"
  "termux-elf-cleaner\0"
  "termux-exec\0"
  "termux-gui-bash\0"
  "termux-gui-c\0"
  "termux-gui-package\0"
  "termux-gui-pm\0"
  "termux-keyring\0"
  "termux-licenses\0"
  "termux-services\0"
  "termux-tools\0"
  "teseq\0"
  "tesseract\0"
  "testssl.sh\0"
  "tex-gyre\0"
  "texinfo\0"
  "texlab\0"
  "texlive-bin\0"
  "texlive-installer\0"
  "tgpt\0"
  "thrift\0"
  "tidy\0"
  "tig\0"
  "tilde\0"
  "time\0"
  "timewarrior\0"
  "timg\0"
  "timidity++\0"
  "tin-summer\0"
  "tintin++\0"
  "tinyfugue\0"
  "tinygo\0"
  "tinymist\0"
  "tinyproxy\0"
  "tinyscheme\0"
  "tinysparql\0"
  "tizonia\0"
  "tk\0"
  "tmate\0"
  "tmux\0"
  "toilet\0"
  "tokei\0"
  "tome2\0"
  "toml11\0"
  "topgrade\0"
  "tor\0"
  "torsocks\0"
  "totem-pl-parser\0"
  "toxic\0"
  "tracepath\0"
  "traceroute\0"
  "translate-shell\0"
  "transmission\0"
  "tree\0"
  "tree-sitter\0"
  "tree-sitter-bash\0"
  "tree-sitter-c\0"
  "tree-sitter-css\0"
  "tree-sitter-go\0"
  "tree-sitter-html\0"
  "tree-sitter-java\0"
  "tree-sitter-javascript\0"
  "tree-sitter-json\0"
  "tree-sitter-latex\0"
  "tree-sitter-lua\0"
  "tree-sitter-markdown\0"
  "tree-sitter-parsers\0"
  "tree-sitter-python\0"
  "tree-sitter-query\0"
  "tree-sitter-regex\0"
  "tree-sitter-rust\0"
  "tree-sitter-sql\0"
  "tree-sitter-toml\0"
  "tree-sitter-vim\0"
  "tree-sitter-vimdoc\0"
  "tree-sitter-xml\0"
  "tree-sitter-yaml\0"
  "trojan-go\0"
  "trunk\0"
  "trurl\0"
  "trzsz-go\0"
  "trzsz-ssh\0"
  "tsocks\0"
  "tsu\0"
  "ttf-dejavu\0"
  "ttf-jetbrains-mono\0"
  "ttf-nerd-fonts-symbols\0"
  "tty-clock\0"
  "tty-solitaire\0"
  "ttyc\0"
  "ttyd\0"
  "ttyper\0"
  "ttyplot\0"
  "ttyrec\0"
  "tudo\0"
  "tur-repo\0"
  "turbo\0"
  "turbopack\0"
  "tut\0"
  "tvheadend\0"
  "tweego\0"
  "txikijs\0"
  "typst\0"
  "typstfmt\0"
  "udftools\0"
  "udocker\0"
  "uftrace\0"
  "ugrep\0"
  "unar\0"
  "unicode-cldr\0"
  "unicode-data\0"
  "unicode-emoji\0"
  "unicorn\0"
  "unifdef\0"
  "units\0"
  "unixodbc\0"
  "unpaper\0"
  "unrar\0"
  "unshield\0"
  "unzip\0"
  "up\0"
  "upower\0"
  "upx\0"
  "urdfdom\0"
  "urdfdom-headers\0"
  "usbmuxd\0"
  "usql\0"
  "utf8cpp\0"
  "utf8proc\0"
  "uthash\0"
  "util-linux\0"
  "uucp\0"
  "uv\0"
  "uwsgi\0"
  "v2ray\0"
  "valac\0"
  "vale\0"
  "valgrind\0"
  "valkey\0"
  "vamp-plugin-sdk\0"
  "vapoursynth\0"
  "vbindiff\0"
  "vcsh\0"
  "vde2\0"
  "vegeta\0"
  "vera\0"
  "vgmstream\0"
  "vgmtools\0"
  "viddy\0"
  "vifm\0"
  "vile\0"
  "vim\0"
  "virglrenderer\0"
  "virglrenderer-android\0"
  "virustotal-cli\0"
  "vis\0"
  "vitetris\0"
  "viu\0"
  "vivid\0"
  "vlc\0"
  "vobsub2srt\0"
  "vorbis-tools\0"
  "vtm\0"
  "vttest\0"
  "vulkan-extension-layer\0"
  "vulkan-headers\0"
  "vulkan-icd\0"
  "vulkan-loader\0"
  "vulkan-loader-android\0"
  "vulkan-loader-generic\0"
  "vulkan-tools\0"
  "vulkan-utility-libraries\0"
  "vulkan-validation-layers\0"
  "vulkan-volk\0"
  "w3m\0"
  "wabt\0"
  "wakatime-cli\0"
  "walk\0"
  "wallust\0"
  "wasi-libc\0"
  "wasm-component-ld\0"
  "wasmedge\0"
  "wasmer\0"
  "wasmtime\0"
  "watchexec\0"
  "waypipe\0"
  "wcalc\0"
  "wdiff\0"
  "webp-pixbuf-loader\0"
  "websocat\0"
  "websocketd\0"
  "webtunnel\0"
  "weechat\0"
  "weechat-matrix-rs\0"
  "weggli\0"
  "wego\0"
  "wget\0"
  "wget2\0"
  "wgetpaste\0"
  "which\0"
  "whitebox-tools\0"
  "whois\0"
  "wireguard-tools\0"
  "wireproxy\0"
  "wiz\0"
  "woff2\0"
  "wol\0"
  "wordgrinder\0"
  "wren\0"
  "wrk\0"
  "wtfutil\0"
  "wuzz\0"
  "x11-repo\0"
  "xcb-proto\0"
  "xdelta3\0"
  "xh\0"
  "xmake\0"
  "xmlsec\0"
  "xmlstarlet\0"
  "xmlto\0"
  "xmppc\0"
  "xorg-util-macros\0"
  "xorgproto\0"
  "xorriso\0"
  "xtrans\0"
  "xvidcore\0"
  "xxhash\0"
  "yadm\0"
  "yajl\0"
  "yara\0"
  "yarn\0"
  "yasm\0"
  "yazi\0"
  "yosys\0"
  "youtubedr\0"
  "yq\0"
  "yt-dlp-ejs\0"
  "ytfzf\0"
  "ytui-music\0"
  "yuma123\0"
  "z-push\0"
  "z3\0"
  "zbar\0"
  "zellij\0"
  "zeronet\0"
  "zig\0"
  "zile\0"
  "zip\0"
  "zipios\0"
  "zk\0"
  "zlib\0"
  "zls\0"
  "znc\0"
  "zola\0"
  "zoxide\0"
  "zpaq\0"
  "zrok\0"
  "zsh\0"
  "zsh-completions\0"
  "zssh\0"
  "zstd\0"
  "zsync\0"
  "zziplib\0"
  "zzuf\0"
  ;

static const u32 g_termux_pkg_name_offs[] = {
  0u,
  9u,
  16u,
  22u,
  30u,
  35u,
  44u,
  50u,
  55u,
  62u,
  68u,
  77u,
  88u,
  97u,
  101u,
  105u,
  110u,
  115u,
  121u,
  125u,
  129u,
  133u,
  140u,
  146u,
  154u,
  163u,
  170u,
  179u,
  192u,
  203u,
  209u,
  216u,
  230u,
  237u,
  245u,
  259u,
  273u,
  281u,
  292u,
  296u,
  305u,
  314u,
  324u,
  335u,
  343u,
  350u,
  360u,
  370u,
  374u,
  383u,
  387u,
  396u,
  414u,
  420u,
  427u,
  432u,
  438u,
  442u,
  452u,
  459u,
  466u,
  472u,
  481u,
  493u,
  503u,
  511u,
  518u,
  528u,
  538u,
  548u,
  558u,
  565u,
  575u,
  578u,
  591u,
  605u,
  612u,
  618u,
  623u,
  629u,
  635u,
  645u,
  654u,
  671u,
  680u,
  689u,
  697u,
  703u,
  708u,
  714u,
  721u,
  726u,
  732u,
  737u,
  747u,
  755u,
  765u,
  770u,
  786u,
  793u,
  797u,
  800u,
  806u,
  811u,
  818u,
  828u,
  838u,
  843u,
  847u,
  853u,
  859u,
  867u,
  876u,
  885u,
  891u,
  899u,
  907u,
  910u,
  919u,
  925u,
  931u,
  950u,
  955u,
  961u,
  970u,
  981u,
  987u,
  992u,
  1003u,
  1010u,
  1016u,
  1026u,
  1033u,
  1039u,
  1045u,
  1052u,
  1063u,
  1073u,
  1079u,
  1091u,
  1095u,
  1111u,
  1119u,
  1123u,
  1129u,
  1135u,
  1142u,
  1152u,
  1168u,
  1182u,
  1193u,
  1199u,
  1207u,
  1213u,
  1218u,
  1227u,
  1237u,
  1246u,
  1255u,
  1263u,
  1276u,
  1283u,
  1291u,
  1298u,
  1303u,
  1318u,
  1324u,
  1331u,
  1339u,
  1344u,
  1351u,
  1356u,
  1368u,
  1373u,
  1380u,
  1389u,
  1393u,
  1398u,
  1403u,
  1408u,
  1414u,
  1420u,
  1428u,
  1436u,
  1443u,
  1450u,
  1457u,
  1464u,
  1469u,
  1475u,
  1483u,
  1490u,
  1498u,
  1505u,
  1511u,
  1518u,
  1524u,
  1532u,
  1544u,
  1551u,
  1556u,
  1562u,
  1568u,
  1576u,
  1583u,
  1588u,
  1595u,
  1605u,
  1611u,
  1622u,
  1633u,
  1641u,
  1646u,
  1656u,
  1674u,
  1683u,
  1698u,
  1709u,
  1717u,
  1727u,
  1733u,
  1743u,
  1753u,
  1760u,
  1765u,
  1774u,
  1779u,
  1787u,
  1796u,
  1805u,
  1811u,
  1816u,
  1823u,
  1832u,
  1839u,
  1848u,
  1856u,
  1863u,
  1867u,
  1872u,
  1879u,
  1885u,
  1895u,
  1904u,
  1909u,
  1918u,
  1925u,
  1936u,
  1940u,
  1943u,
  1953u,
  1957u,
  1967u,
  1972u,
  1978u,
  1983u,
  1988u,
  1997u,
  2007u,
  2012u,
  2024u,
  2030u,
  2036u,
  2045u,
  2057u,
  2069u,
  2075u,
  2080u,
  2086u,
  2093u,
  2101u,
  2108u,
  2114u,
  2128u,
  2137u,
  2148u,
  2158u,
  2163u,
  2170u,
  2179u,
  2188u,
  2197u,
  2204u,
  2212u,
  2219u,
  2229u,
  2239u,
  2250u,
  2257u,
  2263u,
  2276u,
  2284u,
  2295u,
  2305u,
  2312u,
  2319u,
  2328u,
  2340u,
  2352u,
  2359u,
  2367u,
  2372u,
  2381u,
  2390u,
  2401u,
  2409u,
  2421u,
  2432u,
  2442u,
  2452u,
  2470u,
  2478u,
  2483u,
  2489u,
  2498u,
  2504u,
  2508u,
  2512u,
  2516u,
  2520u,
  2524u,
  2529u,
  2534u,
  2544u,
  2549u,
  2557u,
  2560u,
  2570u,
  2578u,
  2590u,
  2594u,
  2598u,
  2601u,
  2610u,
  2630u,
  2636u,
  2640u,
  2655u,
  2664u,
  2671u,
  2678u,
  2686u,
  2693u,
  2699u,
  2710u,
  2721u,
  2729u,
  2737u,
  2746u,
  2751u,
  2758u,
  2766u,
  2773u,
  2776u,
  2781u,
  2790u,
  2801u,
  2808u,
  2817u,
  2823u,
  2830u,
  2834u,
  2841u,
  2850u,
  2855u,
  2865u,
  2873u,
  2881u,
  2887u,
  2895u,
  2899u,
  2902u,
  2909u,
  2913u,
  2922u,
  2929u,
  2936u,
  2948u,
  2958u,
  2962u,
  2969u,
  2987u,
  2994u,
  2999u,
  3006u,
  3011u,
  3017u,
  3027u,
  3037u,
  3042u,
  3048u,
  3060u,
  3065u,
  3076u,
  3083u,
  3086u,
  3090u,
  3101u,
  3109u,
  3117u,
  3124u,
  3134u,
  3144u,
  3153u,
  3168u,
  3181u,
  3189u,
  3198u,
  3204u,
  3208u,
  3216u,
  3222u,
  3229u,
  3232u,
  3236u,
  3240u,
  3255u,
  3259u,
  3267u,
  3274u,
  3279u,
  3283u,
  3288u,
  3293u,
  3298u,
  3302u,
  3307u,
  3318u,
  3336u,
  3340u,
  3352u,
  3359u,
  3364u,
  3374u,
  3388u,
  3404u,
  3414u,
  3422u,
  3427u,
  3435u,
  3442u,
  3449u,
  3456u,
  3459u,
  3463u,
  3475u,
  3482u,
  3491u,
  3498u,
  3502u,
  3525u,
  3535u,
  3545u,
  3556u,
  3564u,
  3574u,
  3583u,
  3589u,
  3601u,
  3610u,
  3616u,
  3624u,
  3633u,
  3639u,
  3644u,
  3660u,
  3671u,
  3675u,
  3682u,
  3687u,
  3692u,
  3700u,
  3709u,
  3716u,
  3721u,
  3724u,
  3731u,
  3740u,
  3749u,
  3755u,
  3761u,
  3768u,
  3774u,
  3782u,
  3788u,
  3797u,
  3804u,
  3817u,
  3835u,
  3847u,
  3856u,
  3863u,
  3885u,
  3890u,
  3895u,
  3902u,
  3907u,
  3914u,
  3926u,
  3937u,
  3943u,
  3950u,
  3957u,
  3963u,
  3971u,
  3976u,
  3983u,
  3989u,
  3999u,
  4005u,
  4010u,
  4016u,
  4022u,
  4030u,
  4036u,
  4045u,
  4052u,
  4060u,
  4065u,
  4074u,
  4089u,
  4098u,
  4104u,
  4109u,
  4114u,
  4120u,
  4125u,
  4133u,
  4140u,
  4148u,
  4152u,
  4162u,
  4178u,
  4195u,
  4212u,
  4229u,
  4240u,
  4250u,
  4258u,
  4264u,
  4268u,
  4281u,
  4300u,
  4305u,
  4313u,
  4322u,
  4335u,
  4344u,
  4348u,
  4355u,
  4361u,
  4367u,
  4372u,
  4380u,
  4389u,
  4398u,
  4406u,
  4412u,
  4418u,
  4423u,
  4432u,
  4439u,
  4447u,
  4455u,
  4466u,
  4474u,
  4484u,
  4489u,
  4494u,
  4509u,
  4519u,
  4524u,
  4531u,
  4539u,
  4547u,
  4551u,
  4556u,
  4563u,
  4572u,
  4587u,
  4599u,
  4611u,
  4623u,
  4635u,
  4639u,
  4646u,
  4656u,
  4666u,
  4669u,
  4674u,
  4678u,
  4686u,
  4691u,
  4709u,
  4718u,
  4725u,
  4731u,
  4743u,
  4749u,
  4756u,
  4763u,
  4773u,
  4782u,
  4794u,
  4808u,
  4817u,
  4822u,
  4829u,
  4836u,
  4845u,
  4854u,
  4863u,
  4875u,
  4885u,
  4890u,
  4896u,
  4902u,
  4911u,
  4916u,
  4921u,
  4940u,
  4946u,
  4954u,
  4959u,
  4968u,
  4977u,
  4982u,
  4998u,
  5008u,
  5014u,
  5020u,
  5026u,
  5034u,
  5040u,
  5043u,
  5047u,
  5052u,
  5057u,
  5067u,
  5070u,
  5077u,
  5081u,
  5088u,
  5098u,
  5106u,
  5111u,
  5116u,
  5121u,
  5128u,
  5137u,
  5141u,
  5158u,
  5166u,
  5178u,
  5182u,
  5190u,
  5199u,
  5204u,
  5216u,
  5223u,
  5228u,
  5235u,
  5240u,
  5250u,
  5258u,
  5268u,
  5273u,
  5284u,
  5297u,
  5305u,
  5310u,
  5314u,
  5318u,
  5323u,
  5330u,
  5335u,
  5345u,
  5350u,
  5359u,
  5367u,
  5374u,
  5381u,
  5384u,
  5393u,
  5398u,
  5413u,
  5419u,
  5426u,
  5433u,
  5440u,
  5464u,
  5484u,
  5500u,
  5527u,
  5546u,
  5563u,
  5580u,
  5596u,
  5615u,
  5641u,
  5659u,
  5678u,
  5684u,
  5691u,
  5707u,
  5718u,
  5731u,
  5739u,
  5746u,
  5756u,
  5770u,
  5778u,
  5793u,
  5802u,
  5812u,
  5822u,
  5830u,
  5837u,
  5847u,
  5855u,
  5862u,
  5869u,
  5885u,
  5897u,
  5905u,
  5914u,
  5929u,
  5945u,
  5952u,
  5962u,
  5969u,
  5977u,
  5984u,
  5991u,
  6007u,
  6019u,
  6034u,
  6041u,
  6048u,
  6056u,
  6070u,
  6084u,
  6100u,
  6122u,
  6132u,
  6143u,
  6158u,
  6167u,
  6176u,
  6183u,
  6192u,
  6200u,
  6208u,
  6218u,
  6226u,
  6235u,
  6241u,
  6250u,
  6261u,
  6270u,
  6282u,
  6298u,
  6311u,
  6319u,
  6326u,
  6336u,
  6347u,
  6357u,
  6367u,
  6377u,
  6388u,
  6396u,
  6407u,
  6415u,
  6422u,
  6430u,
  6439u,
  6451u,
  6462u,
  6468u,
  6477u,
  6485u,
  6494u,
  6502u,
  6509u,
  6520u,
  6527u,
  6539u,
  6547u,
  6556u,
  6568u,
  6578u,
  6587u,
  6596u,
  6602u,
  6612u,
  6618u,
  6632u,
  6639u,
  6647u,
  6658u,
  6666u,
  6677u,
  6685u,
  6699u,
  6714u,
  6723u,
  6732u,
  6739u,
  6746u,
  6762u,
  6772u,
  6785u,
  6797u,
  6805u,
  6814u,
  6821u,
  6829u,
  6836u,
  6844u,
  6854u,
  6862u,
  6870u,
  6878u,
  6889u,
  6900u,
  6910u,
  6918u,
  6925u,
  6934u,
  6941u,
  6951u,
  6958u,
  6966u,
  6980u,
  6997u,
  7019u,
  7028u,
  7036u,
  7049u,
  7058u,
  7069u,
  7078u,
  7087u,
  7096u,
  7107u,
  7117u,
  7131u,
  7138u,
  7151u,
  7160u,
  7168u,
  7178u,
  7186u,
  7197u,
  7210u,
  7224u,
  7232u,
  7240u,
  7246u,
  7255u,
  7266u,
  7273u,
  7281u,
  7288u,
  7296u,
  7303u,
  7310u,
  7317u,
  7331u,
  7340u,
  7352u,
  7365u,
  7375u,
  7381u,
  7389u,
  7402u,
  7412u,
  7421u,
  7435u,
  7446u,
  7460u,
  7467u,
  7478u,
  7491u,
  7502u,
  7513u,
  7520u,
  7533u,
  7542u,
  7550u,
  7560u,
  7571u,
  7586u,
  7598u,
  7609u,
  7619u,
  7627u,
  7634u,
  7644u,
  7652u,
  7659u,
  7666u,
  7675u,
  7686u,
  7697u,
  7707u,
  7713u,
  7721u,
  7729u,
  7737u,
  7744u,
  7751u,
  7760u,
  7767u,
  7777u,
  7784u,
  7792u,
  7799u,
  7811u,
  7821u,
  7837u,
  7848u,
  7859u,
  7867u,
  7878u,
  7885u,
  7895u,
  7905u,
  7912u,
  7921u,
  7928u,
  7943u,
  7952u,
  7967u,
  7983u,
  7993u,
  8002u,
  8010u,
  8022u,
  8032u,
  8042u,
  8054u,
  8064u,
  8075u,
  8084u,
  8093u,
  8100u,
  8113u,
  8121u,
  8129u,
  8141u,
  8155u,
  8168u,
  8175u,
  8186u,
  8198u,
  8209u,
  8217u,
  8227u,
  8236u,
  8243u,
  8250u,
  8274u,
  8292u,
  8301u,
  8312u,
  8320u,
  8332u,
  8343u,
  8354u,
  8362u,
  8371u,
  8381u,
  8391u,
  8405u,
  8413u,
  8421u,
  8431u,
  8439u,
  8450u,
  8463u,
  8473u,
  8481u,
  8490u,
  8504u,
  8518u,
  8539u,
  8550u,
  8559u,
  8572u,
  8581u,
  8587u,
  8597u,
  8608u,
  8618u,
  8629u,
  8639u,
  8653u,
  8661u,
  8670u,
  8678u,
  8694u,
  8708u,
  8718u,
  8729u,
  8738u,
  8756u,
  8772u,
  8782u,
  8789u,
  8796u,
  8804u,
  8815u,
  8826u,
  8838u,
  8853u,
  8862u,
  8874u,
  8886u,
  8896u,
  8905u,
  8914u,
  8921u,
  8927u,
  8934u,
  8945u,
  8955u,
  8968u,
  8976u,
  8986u,
  8994u,
  9005u,
  9017u,
  9026u,
  9036u,
  9048u,
  9059u,
  9067u,
  9078u,
  9099u,
  9107u,
  9121u,
  9133u,
  9143u,
  9154u,
  9165u,
  9176u,
  9188u,
  9200u,
  9211u,
  9222u,
  9235u,
  9248u,
  9259u,
  9267u,
  9274u,
  9285u,
  9297u,
  9303u,
  9310u,
  9320u,
  9331u,
  9340u,
  9348u,
  9357u,
  9365u,
  9380u,
  9390u,
  9400u,
  9407u,
  9416u,
  9423u,
  9434u,
  9445u,
  9466u,
  9474u,
  9501u,
  9515u,
  9526u,
  9533u,
  9540u,
  9547u,
  9555u,
  9564u,
  9570u,
  9577u,
  9585u,
  9593u,
  9603u,
  9610u,
  9617u,
  9626u,
  9637u,
  9652u,
  9661u,
  9672u,
  9680u,
  9690u,
  9697u,
  9703u,
  9710u,
  9724u,
  9732u,
  9740u,
  9750u,
  9760u,
  9771u,
  9784u,
  9792u,
  9799u,
  9805u,
  9813u,
  9819u,
  9830u,
  9838u,
  9850u,
  9857u,
  9864u,
  9872u,
  9879u,
  9897u,
  9904u,
  9911u,
  9921u,
  9934u,
  9943u,
  9948u,
  9957u,
  9963u,
  9968u,
  9972u,
  9986u,
  9996u,
  10006u,
  10014u,
  10029u,
  10050u,
  10055u,
  10059u,
  10068u,
  10076u,
  10086u,
  10092u,
  10100u,
  10103u,
  10109u,
  10115u,
  10119u,
  10124u,
  10129u,
  10136u,
  10156u,
  10164u,
  10173u,
  10179u,
  10185u,
  10191u,
  10197u,
  10203u,
  10210u,
  10221u,
  10230u,
  10234u,
  10239u,
  10245u,
  10249u,
  10257u,
  10261u,
  10268u,
  10273u,
  10282u,
  10287u,
  10293u,
  10298u,
  10301u,
  10310u,
  10328u,
  10337u,
  10347u,
  10352u,
  10363u,
  10370u,
  10377u,
  10383u,
  10392u,
  10402u,
  10410u,
  10417u,
  10437u,
  10446u,
  10457u,
  10468u,
  10481u,
  10492u,
  10500u,
  10517u,
  10523u,
  10530u,
  10537u,
  10544u,
  10552u,
  10555u,
  10561u,
  10568u,
  10592u,
  10608u,
  10620u,
  10636u,
  10649u,
  10666u,
  10681u,
  10699u,
  10713u,
  10729u,
  10762u,
  10776u,
  10791u,
  10803u,
  10814u,
  10823u,
  10828u,
  10838u,
  10842u,
  10854u,
  10864u,
  10873u,
  10881u,
  10891u,
  10901u,
  10909u,
  10914u,
  10920u,
  10923u,
  10929u,
  10940u,
  10947u,
  10955u,
  10967u,
  10975u,
  10984u,
  10993u,
  11003u,
  11009u,
  11019u,
  11028u,
  11038u,
  11047u,
  11057u,
  11065u,
  11070u,
  11080u,
  11088u,
  11093u,
  11100u,
  11108u,
  11113u,
  11122u,
  11133u,
  11138u,
  11142u,
  11152u,
  11158u,
  11170u,
  11175u,
  11180u,
  11187u,
  11197u,
  11205u,
  11213u,
  11221u,
  11225u,
  11229u,
  11241u,
  11249u,
  11253u,
  11259u,
  11266u,
  11273u,
  11282u,
  11288u,
  11298u,
  11305u,
  11308u,
  11317u,
  11322u,
  11332u,
  11338u,
  11359u,
  11364u,
  11370u,
  11376u,
  11392u,
  11400u,
  11412u,
  11416u,
  11421u,
  11426u,
  11431u,
  11436u,
  11444u,
  11449u,
  11459u,
  11464u,
  11469u,
  11475u,
  11481u,
  11489u,
  11499u,
  11509u,
  11516u,
  11524u,
  11537u,
  11549u,
  11552u,
  11558u,
  11570u,
  11579u,
  11587u,
  11594u,
  11609u,
  11617u,
  11626u,
  11636u,
  11651u,
  11660u,
  11668u,
  11675u,
  11682u,
  11713u,
  11722u,
  11731u,
  11737u,
  11744u,
  11752u,
  11756u,
  11762u,
  11772u,
  11786u,
  11792u,
  11797u,
  11801u,
  11806u,
  11810u,
  11826u,
  11833u,
  11844u,
  11854u,
  11862u,
  11866u,
  11872u,
  11877u,
  11884u,
  11891u,
  11899u,
  11907u,
  11914u,
  11923u,
  11931u,
  11937u,
  11944u,
  11952u,
  11960u,
  11971u,
  11976u,
  11987u,
  11990u,
  11995u,
  12002u,
  12006u,
  12015u,
  12022u,
  12032u,
  12044u,
  12051u,
  12066u,
  12078u,
  12088u,
  12101u,
  12116u,
  12137u,
  12149u,
  12157u,
  12166u,
  12173u,
  12182u,
  12193u,
  12204u,
  12215u,
  12224u,
  12233u,
  12242u,
  12250u,
  12258u,
  12265u,
  12274u,
  12282u,
  12290u,
  12305u,
  12312u,
  12320u,
  12331u,
  12340u,
  12349u,
  12357u,
  12367u,
  12377u,
  12389u,
  12402u,
  12412u,
  12417u,
  12424u,
  12432u,
  12438u,
  12445u,
  12460u,
  12468u,
  12475u,
  12481u,
  12490u,
  12495u,
  12504u,
  12509u,
  12516u,
  12522u,
  12527u,
  12536u,
  12544u,
  12559u,
  12570u,
  12577u,
  12583u,
  12592u,
  12603u,
  12606u,
  12611u,
  12622u,
  12627u,
  12633u,
  12641u,
  12648u,
  12656u,
  12662u,
  12671u,
  12676u,
  12681u,
  12693u,
  12697u,
  12704u,
  12713u,
  12724u,
  12728u,
  12737u,
  12749u,
  12757u,
  12767u,
  12785u,
  12796u,
  12805u,
  12810u,
  12818u,
  12827u,
  12832u,
  12841u,
  12850u,
  12857u,
  12868u,
  12877u,
  12886u,
  12897u,
  12905u,
  12912u,
  12921u,
  12932u,
  12938u,
  12947u,
  12956u,
  12967u,
  12972u,
  12979u,
  12986u,
  13002u,
  13010u,
  13023u,
  13033u,
  13042u,
  13052u,
  13060u,
  13071u,
  13079u,
  13086u,
  13093u,
  13101u,
  13109u,
  13116u,
  13122u,
  13141u,
  13151u,
  13160u,
  13165u,
  13171u,
  13184u,
  13198u,
  13206u,
  13216u,
  13231u,
  13238u,
  13243u,
  13254u,
  13260u,
  13271u,
  13275u,
  13285u,
  13288u,
  13294u,
  13303u,
  13311u,
  13321u,
  13326u,
  13332u,
  13340u,
  13349u,
  13356u,
  13368u,
  13379u,
  13393u,
  13407u,
  13420u,
  13437u,
  13457u,
  13473u,
  13487u,
  13502u,
  13517u,
  13533u,
  13545u,
  13560u,
  13573u,
  13587u,
  13598u,
  13612u,
  13633u,
  13647u,
  13663u,
  13676u,
  13696u,
  13718u,
  13730u,
  13743u,
  13761u,
  13779u,
  13798u,
  13810u,
  13824u,
  13832u,
  13845u,
  13850u,
  13878u,
  13884u,
  13889u,
  13897u,
  13909u,
  13923u,
  13934u,
  13940u,
  13956u,
  13963u,
  13971u,
  13976u,
  13982u,
  13994u,
  14003u,
  14010u,
  14020u,
  14025u,
  14035u,
  14039u,
  14042u,
  14049u,
  14053u,
  14057u,
  14064u,
  14077u,
  14084u,
  14092u,
  14097u,
  14106u,
  14113u,
  14120u,
  14129u,
  14135u,
  14141u,
  14148u,
  14160u,
  14167u,
  14179u,
  14186u,
  14200u,
  14208u,
  14214u,
  14220u,
  14224u,
  14231u,
  14236u,
  14244u,
  14256u,
  14267u,
  14272u,
  14278u,
  14285u,
  14290u,
  14294u,
  14304u,
  14321u,
  14331u,
  14340u,
  14344u,
  14347u,
  14354u,
  14364u,
  14370u,
  14379u,
  14388u,
  14399u,
  14411u,
  14416u,
  14421u,
  14427u,
  14432u,
  14437u,
  14451u,
  14464u,
  14473u,
  14481u,
  14489u,
  14495u,
  14504u,
  14512u,
  14518u,
  14523u,
  14529u,
  14535u,
  14539u,
  14547u,
  14553u,
  14560u,
  14572u,
  14578u,
  14585u,
  14588u,
  14593u,
  14608u,
  14616u,
  14624u,
  14638u,
  14642u,
  14649u,
  14658u,
  14665u,
  14680u,
  14685u,
  14690u,
  14696u,
  14703u,
  14711u,
  14721u,
  14725u,
  14733u,
  14744u,
  14755u,
  14767u,
  14779u,
  14785u,
  14792u,
  14800u,
  14807u,
  14815u,
  14823u,
  14841u,
  14847u,
  14856u,
  14861u,
  14870u,
  14879u,
  14885u,
  14888u,
  14894u,
  14904u,
  14914u,
  14921u,
  14929u,
  14939u,
  14955u,
  14962u,
  14972u,
  14978u,
  14992u,
  14997u,
  15006u,
  15011u,
  15016u,
  15040u,
  15044u,
  15061u,
  15069u,
  15082u,
  15091u,
  15098u,
  15111u,
  15118u,
  15132u,
  15154u,
  15166u,
  15176u,
  15194u,
  15206u,
  15212u,
  15219u,
  15226u,
  15237u,
  15242u,
  15249u,
  15258u,
  15266u,
  15274u,
  15282u,
  15290u,
  15295u,
  15300u,
  15309u,
  15316u,
  15329u,
  15338u,
  15347u,
  15352u,
  15362u,
  15369u,
  15375u,
  15388u,
  15393u,
  15400u,
  15413u,
  15421u,
  15430u,
  15437u,
  15452u,
  15463u,
  15468u,
  15473u,
  15483u,
  15495u,
  15499u,
  15508u,
  15522u,
  15530u,
  15538u,
  15544u,
  15555u,
  15561u,
  15573u,
  15578u,
  15584u,
  15594u,
  15602u,
  15609u,
  15616u,
  15622u,
  15626u,
  15639u,
  15646u,
  15658u,
  15662u,
  15666u,
  15673u,
  15678u,
  15682u,
  15686u,
  15695u,
  15702u,
  15711u,
  15728u,
  15741u,
  15746u,
  15754u,
  15764u,
  15773u,
  15783u,
  15800u,
  15811u,
  15827u,
  15839u,
  15851u,
  15873u,
  15892u,
  15904u,
  15920u,
  15933u,
  15952u,
  15966u,
  15981u,
  15997u,
  16013u,
  16026u,
  16032u,
  16042u,
  16053u,
  16062u,
  16070u,
  16077u,
  16089u,
  16107u,
  16112u,
  16119u,
  16124u,
  16128u,
  16134u,
  16139u,
  16151u,
  16156u,
  16167u,
  16178u,
  16187u,
  16197u,
  16204u,
  16213u,
  16223u,
  16234u,
  16245u,
  16253u,
  16256u,
  16262u,
  16267u,
  16274u,
  16280u,
  16286u,
  16293u,
  16302u,
  16306u,
  16315u,
  16331u,
  16337u,
  16347u,
  16358u,
  16374u,
  16387u,
  16392u,
  16404u,
  16421u,
  16435u,
  16451u,
  16466u,
  16483u,
  16500u,
  16523u,
  16540u,
  16558u,
  16574u,
  16595u,
  16615u,
  16634u,
  16652u,
  16670u,
  16687u,
  16703u,
  16720u,
  16736u,
  16755u,
  16771u,
  16788u,
  16798u,
  16804u,
  16810u,
  16819u,
  16829u,
  16836u,
  16840u,
  16851u,
  16870u,
  16893u,
  16903u,
  16917u,
  16922u,
  16927u,
  16934u,
  16942u,
  16949u,
  16954u,
  16963u,
  16969u,
  16979u,
  16983u,
  16993u,
  17000u,
  17008u,
  17014u,
  17023u,
  17032u,
  17040u,
  17048u,
  17054u,
  17059u,
  17072u,
  17085u,
  17099u,
  17107u,
  17115u,
  17121u,
  17130u,
  17138u,
  17144u,
  17153u,
  17159u,
  17162u,
  17169u,
  17173u,
  17181u,
  17197u,
  17205u,
  17210u,
  17218u,
  17227u,
  17234u,
  17245u,
  17250u,
  17253u,
  17259u,
  17265u,
  17271u,
  17276u,
  17285u,
  17292u,
  17308u,
  17320u,
  17329u,
  17334u,
  17339u,
  17346u,
  17351u,
  17361u,
  17370u,
  17376u,
  17381u,
  17386u,
  17390u,
  17404u,
  17426u,
  17441u,
  17445u,
  17454u,
  17458u,
  17464u,
  17468u,
  17479u,
  17492u,
  17496u,
  17503u,
  17526u,
  17541u,
  17552u,
  17566u,
  17588u,
  17610u,
  17623u,
  17648u,
  17673u,
  17685u,
  17689u,
  17694u,
  17707u,
  17712u,
  17720u,
  17730u,
  17748u,
  17757u,
  17764u,
  17773u,
  17783u,
  17791u,
  17797u,
  17803u,
  17822u,
  17831u,
  17842u,
  17852u,
  17860u,
  17878u,
  17885u,
  17890u,
  17895u,
  17901u,
  17911u,
  17917u,
  17932u,
  17938u,
  17954u,
  17964u,
  17968u,
  17974u,
  17978u,
  17990u,
  17995u,
  17999u,
  18007u,
  18012u,
  18021u,
  18031u,
  18039u,
  18042u,
  18048u,
  18055u,
  18066u,
  18072u,
  18078u,
  18095u,
  18105u,
  18113u,
  18120u,
  18129u,
  18136u,
  18141u,
  18146u,
  18151u,
  18156u,
  18161u,
  18166u,
  18172u,
  18182u,
  18185u,
  18196u,
  18202u,
  18213u,
  18221u,
  18228u,
  18231u,
  18236u,
  18243u,
  18251u,
  18255u,
  18260u,
  18264u,
  18271u,
  18274u,
  18279u,
  18283u,
  18287u,
  18292u,
  18299u,
  18304u,
  18309u,
  18313u,
  18329u,
  18334u,
  18339u,
  18345u,
  18353u,
};
#else
#define RAF_PKG_INIT(ID, NLEN, FLG, VFY, NPTR) { (ID), (NLEN), (FLG), (VFY) }
#endif

static const raf_termux_pkg_id_t g_termux_pkgs[] = {
  { 0x7baf4038u, 8u, 0u }, /* 0verkill */
  { 0xb09d2723u, 6u, 0u }, /* 2048-c */
  { 0x575564b9u, 5u, 0u }, /* 2ping */
  { 0xda4d003bu, 7u, 0u }, /* 6tunnel */
  { 0xfcfe8405u, 4u, 0u }, /* 7zip */
  { 0xadebb3e5u, 8u, 0u }, /* 8086tiny */
  { 0x10675f6eu, 5u, 0u }, /* aalib */
  { 0x5dc677a3u, 4u, 0u }, /* aapt */
  { 0x28df39dfu, 6u, 0u }, /* abduco */
  { 0xb632a181u, 5u, 0u }, /* abook */
  { 0x83b92de3u, 8u, 0u }, /* abootimg */
  { 0x85af1ba7u, 10u, 0u }, /* abseil-cpp */
  { 0x0c32e369u, 8u, 0u }, /* ack-grep */
  { 0x334a4efdu, 3u, 0u }, /* acr */
  { 0x38390dbbu, 3u, 0u }, /* ada */
  { 0x51c63114u, 4u, 0u }, /* adms */
  { 0xc638b6a8u, 4u, 0u }, /* aerc */
  { 0x980bdf6bu, 5u, 0u }, /* agate */
  { 0x2c41499cu, 3u, 0u }, /* age */
  { 0x2e414cc2u, 3u, 0u }, /* agg */
  { 0x202fed97u, 3u, 0u }, /* aha */
  { 0xe48a2881u, 6u, 0u }, /* aichat */
  { 0xfbe97f09u, 5u, 0u }, /* alass */
  { 0x2a178da4u, 7u, 0u }, /* alembic */
  { 0x7f46d551u, 8u, 0u }, /* algernon */
  { 0x7d538ddeu, 6u, 0u }, /* alpine */
  { 0xc4359436u, 8u, 0u }, /* alsa-lib */
  { 0xc7e1e725u, 12u, 0u }, /* alsa-plugins */
  { 0x53909808u, 10u, 0u }, /* alsa-utils */
  { 0x6fd4bec0u, 5u, 0u }, /* amber */
  { 0xa6fe3f35u, 6u, 0u }, /* amfora */
  { 0x951cbe8au, 13u, 0u }, /* android-tools */
  { 0xc24f769du, 6u, 0u }, /* anewer */
  { 0xe1b6cdf4u, 7u, 0u }, /* angband */
  { 0x04c917eeu, 13u, 0u }, /* angle-android */
  { 0x72eb13b4u, 13u, 0u }, /* angle-grinder */
  { 0xba16c8a2u, 7u, 0u }, /* ani-cli */
  { 0xa277b92cu, 10u, 0u }, /* ansifilter */
  { 0x1f29dbd6u, 3u, 0u }, /* ant */
  { 0x82d38dbdu, 8u, 0u }, /* antibody */
  { 0xe6d746b5u, 8u, 0u }, /* antiword */
  { 0x5877e06du, 9u, 0u }, /* aosp-libs */
  { 0xbd4680ccu, 10u, 0u }, /* apache-orc */
  { 0x03a707f3u, 7u, 0u }, /* apache2 */
  { 0xb43188d5u, 6u, 0u }, /* apkeep */
  { 0xad57fe9bu, 9u, 0u }, /* apksigner */
  { 0xbb019554u, 9u, 0u }, /* appstream */
  { 0x216a8652u, 3u, 0u }, /* apr */
  { 0x1df1f291u, 8u, 0u }, /* apr-util */
  { 0x1b6a7ce0u, 3u, 0u }, /* apt */
  { 0xea5899a9u, 8u, 0u }, /* apt-file */
  { 0xf31c0bccu, 17u, 0u }, /* apt-show-versions */
  { 0x781f5da7u, 5u, 0u }, /* aptly */
  { 0xe264ce90u, 6u, 0u }, /* argon2 */
  { 0xa00aabf5u, 4u, 0u }, /* argp */
  { 0xfb62789eu, 5u, 0u }, /* aria2 */
  { 0x316f1cb0u, 3u, 0u }, /* arj */
  { 0x220319e1u, 9u, 0u }, /* arpack-ng */
  { 0x73fdf8a4u, 6u, 0u }, /* artalk */
  { 0xf2125b06u, 6u, 0u }, /* arturo */
  { 0x11f85eb0u, 5u, 0u }, /* ascii */
  { 0x03c3377eu, 8u, 0u }, /* asciidoc */
  { 0xb8d8e0e3u, 11u, 0u }, /* asciidoctor */
  { 0x958d1913u, 9u, 0u }, /* asciinema */
  { 0x7d6c6572u, 7u, 0u }, /* asm-lsp */
  { 0xb4b115b6u, 6u, 0u }, /* aspell */
  { 0x2935e76eu, 9u, 0u }, /* aspell-de */
  { 0xfe336526u, 9u, 0u }, /* aspell-en */
  { 0x1b3392cdu, 9u, 0u }, /* aspell-es */
  { 0x0c3bc9f5u, 9u, 0u }, /* aspell-fr */
  { 0x8bbcfcd4u, 6u, 0u }, /* assimp */
  { 0x7eb50a73u, 9u, 0u }, /* asymptote */
  { 0x57251588u, 2u, 0u }, /* at */
  { 0x6368ee81u, 12u, 0u }, /* at-spi2-core */
  { 0x65b13e8eu, 13u, 0u }, /* atomicparsley */
  { 0xd7b01f85u, 6u, 0u }, /* atomvm */
  { 0x2812f6d6u, 5u, 0u }, /* atool */
  { 0x10a838b2u, 4u, 0u }, /* attr */
  { 0x614cc34cu, 5u, 0u }, /* atuin */
  { 0x72f918bbu, 5u, 0u }, /* aubio */
  { 0xcb8177dfu, 9u, 0u }, /* audiofile */
  { 0x40d3e6e4u, 8u, 0u }, /* autoconf */
  { 0xf776d733u, 16u, 0u }, /* autoconf-archive */
  { 0xb26875deu, 8u, 0u }, /* autojump */
  { 0xa1748b3cu, 8u, 0u }, /* automake */
  { 0xa2de9b64u, 7u, 0u }, /* autossh */
  { 0xbb9f4335u, 5u, 0u }, /* aview */
  { 0xf8c7d96fu, 4u, 0u }, /* avra */
  { 0xdc0bbf8du, 5u, 0u }, /* await */
  { 0xab044050u, 6u, 0u }, /* awscli */
  { 0x74364655u, 4u, 0u }, /* axel */
  { 0xafc06ebdu, 5u, 0u }, /* b3sum */
  { 0x38b1abf2u, 4u, 0u }, /* babl */
  { 0x1d923ca4u, 9u, 0u }, /* bacula-fd */
  { 0x2811fd57u, 7u, 0u }, /* barcode */
  { 0xf99695c8u, 9u, 0u }, /* base16384 */
  { 0x3adc901fu, 4u, 0u }, /* bash */
  { 0xd3ec2954u, 15u, 0u }, /* bash-completion */
  { 0x6c5ef5eau, 6u, 0u }, /* bastet */
  { 0x70b773a8u, 3u, 0u }, /* bat */
  { 0x3e2ba9f2u, 2u, 0u }, /* bc */
  { 0x05d1ea82u, 5u, 0u }, /* bc-gh */
  { 0x0fddfddfu, 4u, 0u }, /* bcal */
  { 0x2f5fc862u, 6u, 0u }, /* bchunk */
  { 0xa83d748du, 9u, 0u }, /* bdsup2sub */
  { 0x83a174fbu, 9u, 0u }, /* beanshell */
  { 0x8a524c53u, 4u, 0u }, /* bear */
  { 0x50ae46ecu, 3u, 0u }, /* bed */
  { 0x3fb3b273u, 5u, 0u }, /* bftpd */
  { 0xb51696f1u, 5u, 0u }, /* bgrep */
  { 0x90e3bd58u, 7u, 0u }, /* biboumi */
  { 0xdaac38efu, 8u, 0u }, /* binaryen */
  { 0x526286bbu, 8u, 0u }, /* binutils */
  { 0x900c5a40u, 5u, 0u }, /* bison */
  { 0x55f7427du, 7u, 0u }, /* bitcoin */
  { 0xbdef1f40u, 7u, 0u }, /* bitlbee */
  { 0x462bb68au, 2u, 0u }, /* bk */
  { 0xb044c34du, 8u, 0u }, /* blackbox */
  { 0x07617589u, 5u, 0u }, /* blink */
  { 0xdb0216d6u, 5u, 0u }, /* blogc */
  { 0xca1ed16au, 18u, 0u }, /* blueprint-compiler */
  { 0x0d82b2e1u, 4u, 0u }, /* bmon */
  { 0x73650e46u, 5u, 0u }, /* boinc */
  { 0xd993b0b0u, 8u, 0u }, /* boinctui */
  { 0xcaa8e760u, 10u, 0u }, /* bombadillo */
  { 0xf8e9242cu, 5u, 0u }, /* boost */
  { 0xbb87b4d3u, 4u, 0u }, /* bore */
  { 0x726f5b4bu, 10u, 0u }, /* borgbackup */
  { 0x0efc679eu, 6u, 0u }, /* botan3 */
  { 0x514c98f2u, 5u, 0u }, /* boxes */
  { 0x117a5234u, 9u, 0u }, /* brainfuck */
  { 0x55680a0du, 6u, 0u }, /* brogue */
  { 0x2b820528u, 5u, 0u }, /* brook */
  { 0x1a81ea65u, 5u, 0u }, /* broot */
  { 0x6da9909du, 6u, 0u }, /* brotli */
  { 0x6e14f2e4u, 10u, 0u }, /* bsd-finger */
  { 0x2e968b64u, 9u, 0u }, /* bsd-games */
  { 0xfd08d422u, 5u, 0u }, /* btfs2 */
  { 0xa60484f4u, 11u, 0u }, /* btrfs-progs */
  { 0x72d58be2u, 3u, 0u }, /* buf */
  { 0xa12e5dfeu, 15u, 0u }, /* build-essential */
  { 0xe3282e09u, 7u, 0u }, /* busybox */
  { 0x55dc1a00u, 3u, 0u }, /* bvi */
  { 0x348a6fc5u, 5u, 0u }, /* byacc */
  { 0xdbfe24e4u, 5u, 0u }, /* byobu */
  { 0x40c33102u, 6u, 0u }, /* c-ares */
  { 0xeff0a74bu, 9u, 0u }, /* c-toxcore */
  { 0x36f64460u, 15u, 0u }, /* ca-certificates */
  { 0x655c9c1au, 13u, 0u }, /* cabal-install */
  { 0xdd69276au, 10u, 0u }, /* cabextract */
  { 0x97cbb392u, 5u, 0u }, /* cabin */
  { 0xb10cd42bu, 7u, 0u }, /* cadaver */
  { 0x7bde8210u, 5u, 0u }, /* caddy */
  { 0xbcf192d4u, 4u, 0u }, /* calc */
  { 0x13cf901fu, 8u, 0u }, /* calcurse */
  { 0x1949031bu, 9u, 0u }, /* capnproto */
  { 0x0b300438u, 8u, 0u }, /* capstone */
  { 0x0d5e7e3fu, 8u, 0u }, /* carapace */
  { 0xf2a9a5e7u, 7u, 0u }, /* cargo-c */
  { 0xd2dac0dfu, 12u, 0u }, /* cargo-leptos */
  { 0x684c2333u, 6u, 0u }, /* catdoc */
  { 0xb31d0b15u, 7u, 0u }, /* catgirl */
  { 0x1ad3a506u, 6u, 0u }, /* catimg */
  { 0x9b3204a4u, 4u, 0u }, /* cava */
  { 0x2df33549u, 14u, 0u }, /* cavez-of-phear */
  { 0x21aa0edeu, 5u, 0u }, /* cavif */
  { 0x0bdb4e32u, 6u, 0u }, /* cboard */
  { 0xc60585acu, 7u, 0u }, /* cbonsai */
  { 0x915ac74eu, 4u, 0u }, /* cc65 */
  { 0x200d62d0u, 6u, 0u }, /* ccache */
  { 0xabc4e599u, 4u, 0u }, /* cccc */
  { 0xe988811du, 11u, 0u }, /* ccextractor */
  { 0xd3e9f56au, 4u, 0u }, /* ccls */
  { 0xefc435eau, 6u, 0u }, /* ccrypt */
  { 0xe98bebb6u, 8u, 0u }, /* cfengine */
  { 0x0d76a5a3u, 3u, 0u }, /* cfm */
  { 0x6b283192u, 4u, 0u }, /* cgal */
  { 0x7f200249u, 4u, 0u }, /* cgdb */
  { 0x8113397cu, 4u, 0u }, /* cgif */
  { 0x33c73920u, 5u, 0u }, /* chafa */
  { 0xb3711c47u, 5u, 0u }, /* check */
  { 0xbf320178u, 7u, 0u }, /* chezmoi */
  { 0xf161ff1cu, 7u, 0u }, /* chicken */
  { 0x4ad92969u, 6u, 0u }, /* chntpw */
  { 0x46ff9c40u, 6u, 0u }, /* choose */
  { 0x2c1fa60cu, 6u, 0u }, /* chrony */
  { 0x39fef3d6u, 6u, 0u }, /* cicada */
  { 0x17bc423bu, 4u, 0u }, /* ciso */
  { 0xaf16b94cu, 5u, 0u }, /* cjson */
  { 0x98a1fdb2u, 7u, 0u }, /* ckermit */
  { 0x8dd3244bu, 6u, 0u }, /* clamav */
  { 0xd4d2bd88u, 7u, 0u }, /* clblast */
  { 0xd974e38cu, 6u, 0u }, /* clidle */
  { 0x568b6af0u, 5u, 0u }, /* clifm */
  { 0x755e6422u, 6u, 0u }, /* clinfo */
  { 0x4fa364d1u, 5u, 0u }, /* clipp */
  { 0x0491075fu, 7u, 0u }, /* cloneit */
  { 0x33068c5eu, 11u, 0u }, /* cloudflared */
  { 0xe4565df1u, 6u, 0u }, /* clpeak */
  { 0x44002e3du, 4u, 0u }, /* clvk */
  { 0x6be07ca8u, 5u, 0u }, /* cmake */
  { 0x41f183abu, 5u, 0u }, /* cmark */
  { 0x6293923fu, 7u, 0u }, /* cmatrix */
  { 0xef70a987u, 6u, 0u }, /* cmocka */
  { 0xc0864821u, 4u, 0u }, /* cmus */
  { 0xf3134d78u, 6u, 0u }, /* cmusfm */
  { 0x5e2f0c28u, 9u, 0u }, /* codecrypt */
  { 0xc2c04c84u, 5u, 0u }, /* codon */
  { 0x6b314042u, 10u, 0u }, /* coinor-cbc */
  { 0x6635b591u, 10u, 0u }, /* coinor-clp */
  { 0x60f71bafu, 7u, 0u }, /* cointop */
  { 0xf144e814u, 4u, 0u }, /* colm */
  { 0x2fba9aefu, 9u, 0u }, /* colordiff */
  { 0x44f58687u, 17u, 0u }, /* command-not-found */
  { 0x8a35b313u, 8u, 0u }, /* composer */
  { 0x26859250u, 14u, 0u }, /* console-bridge */
  { 0xb031bda1u, 10u, 0u }, /* convertlit */
  { 0x72850e2du, 7u, 0u }, /* cookcli */
  { 0x4a02fe7bu, 9u, 0u }, /* coreutils */
  { 0x868cb423u, 5u, 0u }, /* corgi */
  { 0x51cd4300u, 9u, 0u }, /* corkscrew */
  { 0x93f80051u, 9u, 0u }, /* corrosion */
  { 0xbcd8db43u, 6u, 0u }, /* cowsay */
  { 0x76fd0a16u, 4u, 0u }, /* cpio */
  { 0x153d7642u, 8u, 0u }, /* cppcheck */
  { 0x550e1db1u, 4u, 0u }, /* cppi */
  { 0xc54a3b44u, 7u, 0u }, /* cppunit */
  { 0xe3e0c0a7u, 8u, 0u }, /* cpufetch */
  { 0x33cf95f8u, 8u, 0u }, /* cpulimit */
  { 0x1e565680u, 5u, 0u }, /* crawl */
  { 0xa7632adau, 4u, 0u }, /* croc */
  { 0xf7ba971fu, 6u, 0u }, /* cronie */
  { 0x3f8170d3u, 8u, 0u }, /* crowbook */
  { 0xc398c3c8u, 6u, 0u }, /* crunch */
  { 0x9331bf30u, 8u, 0u }, /* cryptopp */
  { 0x4594fdc9u, 7u, 0u }, /* crystal */
  { 0x725d9f06u, 6u, 0u }, /* cscope */
  { 0x0e474901u, 3u, 0u }, /* csh */
  { 0x053b0468u, 4u, 0u }, /* csol */
  { 0x110e8616u, 6u, 0u }, /* csview */
  { 0xcb0129dbu, 5u, 0u }, /* ctags */
  { 0xabdda729u, 9u, 0u }, /* ctypes-sh */
  { 0xe985e58bu, 8u, 0u }, /* cuetools */
  { 0x91b49f34u, 4u, 0u }, /* cups */
  { 0xaf89148bu, 8u, 0u }, /* cups-pdf */
  { 0x05c26fe9u, 6u, 0u }, /* curlie */
  { 0x2cac0652u, 10u, 0u }, /* curseofwar */
  { 0x0b4f930du, 3u, 0u }, /* cvs */
  { 0x911d2211u, 2u, 0u }, /* d8 */
  { 0x568aaa9du, 9u, 0u }, /* daemonize */
  { 0xd94f10acu, 3u, 0u }, /* dar */
  { 0xd4f80a53u, 9u, 0u }, /* darkhttpd */
  { 0xef778408u, 4u, 0u }, /* dart */
  { 0xcde18a66u, 5u, 0u }, /* dasel */
  { 0x0179def5u, 4u, 0u }, /* dash */
  { 0xfc79d716u, 4u, 0u }, /* dasm */
  { 0x48008990u, 8u, 0u }, /* datamash */
  { 0x1874cfb2u, 9u, 0u }, /* dateutils */
  { 0x71eceb63u, 4u, 0u }, /* dbus */
  { 0x1b58d1ccu, 11u, 0u }, /* dbus-python */
  { 0x502b0ca6u, 5u, 0u }, /* dcmtk */
  { 0xdfbc2ef6u, 5u, 0u }, /* dcraw */
  { 0x0f96091cu, 8u, 0u }, /* ddrescue */
  { 0x39207cedu, 11u, 0u }, /* debianutils */
  { 0x3eff6218u, 11u, 0u }, /* debootstrap */
  { 0x5afb52c3u, 5u, 0u }, /* delve */
  { 0x8ef64711u, 4u, 0u }, /* deno */
  { 0xa5c8ae42u, 5u, 0u }, /* desed */
  { 0x1c30b14eu, 6u, 0u }, /* deutex */
  { 0x33a28669u, 7u, 0u }, /* dex2jar */
  { 0x0aa7f0b9u, 6u, 0u }, /* dialog */
  { 0x589f9d67u, 5u, 0u }, /* dictd */
  { 0x21c23cb7u, 13u, 0u }, /* diff-so-fancy */
  { 0x5208cdb0u, 8u, 0u }, /* diffstat */
  { 0x10a2ee6au, 10u, 0u }, /* difftastic */
  { 0x2ebea2b7u, 9u, 0u }, /* diffutils */
  { 0x5c5e2642u, 4u, 0u }, /* dirb */
  { 0x6405cb93u, 6u, 0u }, /* direnv */
  { 0x8dd36b6cu, 8u, 0u }, /* direvent */
  { 0x8d308ce0u, 8u, 0u }, /* discordo */
  { 0x127e5c5eu, 8u, 0u }, /* discount */
  { 0x4800db50u, 6u, 0u }, /* diskus */
  { 0xa28f8042u, 7u, 0u }, /* distant */
  { 0x6a30c127u, 6u, 0u }, /* distcc */
  { 0xdec45400u, 9u, 0u }, /* djvulibre */
  { 0xa2f82075u, 9u, 0u }, /* dmagnetic */
  { 0x4f718f4au, 10u, 0u }, /* dmtx-utils */
  { 0xb52e1067u, 6u, 0u }, /* dnglab */
  { 0xa3fd6473u, 5u, 0u }, /* dnote */
  { 0x9c51542du, 12u, 0u }, /* dnote-server */
  { 0xd2f3f311u, 7u, 0u }, /* dns2tcp */
  { 0x3af3efc1u, 10u, 0u }, /* dnscontrol */
  { 0x34dcbc82u, 9u, 0u }, /* dnslookup */
  { 0x92cdac02u, 6u, 0u }, /* dnsmap */
  { 0xbb74c0ffu, 6u, 0u }, /* dnstop */
  { 0xe23a1bc9u, 8u, 0u }, /* dnsutils */
  { 0x10744afcu, 11u, 0u }, /* docbook-xml */
  { 0xf498442au, 11u, 0u }, /* docbook-xsl */
  { 0x53279daau, 6u, 0u }, /* docopt */
  { 0xc74c21d5u, 7u, 0u }, /* doctest */
  { 0x22e23104u, 4u, 0u }, /* doge */
  { 0xf417a23cu, 8u, 0u }, /* dopewars */
  { 0x6695d753u, 8u, 0u }, /* dos2unix */
  { 0x6dc7d6d1u, 10u, 0u }, /* dosfstools */
  { 0x1fd8fee6u, 7u, 0u }, /* dotconf */
  { 0xff550f3cu, 11u, 0u }, /* dotnet-host */
  { 0xe722b1e4u, 10u, 0u }, /* dotnet10.0 */
  { 0xb5b74a2fu, 9u, 0u }, /* dotnet8.0 */
  { 0x4f78b608u, 9u, 0u }, /* dotnet9.0 */
  { 0xf8f8bacfu, 17u, 0u }, /* double-conversion */
  { 0x11558123u, 7u, 0u }, /* doxygen */
  { 0x91ad2963u, 4u, 0u }, /* dpkg */
  { 0xb08349a0u, 5u, 0u }, /* draco */
  { 0x89d973f2u, 8u, 0u }, /* dropbear */
  { 0x3096b8afu, 5u, 0u }, /* dtach */
  { 0xca34b592u, 3u, 0u }, /* dtc */
  { 0xc434ac20u, 3u, 0u }, /* dte */
  { 0xe4310ce9u, 3u, 0u }, /* dua */
  { 0xe23109c3u, 3u, 0u }, /* duc */
  { 0xe5310e7cu, 3u, 0u }, /* duf */
  { 0xdb39219du, 4u, 0u }, /* dufs */
  { 0xca0b3ba5u, 4u, 0u }, /* dust */
  { 0x17895ebau, 9u, 0u }, /* dvdauthor */
  { 0x70bb72d4u, 4u, 0u }, /* dvtm */
  { 0x5f1a197bu, 7u, 0u }, /* dwarves */
  { 0x511cbd51u, 2u, 0u }, /* dx */
  { 0x09f2bfb6u, 9u, 0u }, /* e2fsprogs */
  { 0x15f00b09u, 7u, 0u }, /* e2tools */
  { 0x0ade0603u, 11u, 0u }, /* ebook-tools */
  { 0x7e72bea9u, 3u, 0u }, /* ecj */
  { 0x8072c1cfu, 3u, 0u }, /* ecl */
  { 0x371a55ccu, 2u, 0u }, /* ed */
  { 0x347fb5aeu, 8u, 0u }, /* edbrowse */
  { 0x57037622u, 19u, 0u }, /* editorconfig-core-c */
  { 0xf0deda07u, 5u, 0u }, /* eigen */
  { 0x8f84228du, 3u, 0u }, /* eja */
  { 0xd2007d52u, 14u, 0u }, /* electric-fence */
  { 0xf7bde250u, 8u, 0u }, /* electrum */
  { 0x70fdd359u, 6u, 0u }, /* elinks */
  { 0x483cfa2au, 6u, 0u }, /* elixir */
  { 0x68c2d304u, 7u, 0u }, /* eltclsh */
  { 0x61b99fd6u, 6u, 0u }, /* elvish */
  { 0x6f6d9160u, 5u, 0u }, /* emacs */
  { 0x8997b361u, 10u, 0u }, /* emmylua-ls */
  { 0x8679e093u, 10u, 0u }, /* emscripten */
  { 0x318f6211u, 7u, 0u }, /* enblend */
  { 0xf21d6772u, 7u, 0u }, /* enchant */
  { 0x03b653a1u, 8u, 0u }, /* enscript */
  { 0x966a9278u, 4u, 0u }, /* entr */
  { 0x0544f88au, 6u, 0u }, /* erlang */
  { 0xc81d5097u, 7u, 0u }, /* esbuild */
  { 0x28b4b87eu, 6u, 0u }, /* espeak */
  { 0x471a6efcu, 2u, 0u }, /* et */
  { 0x31a6092fu, 4u, 0u }, /* etsh */
  { 0xed670d63u, 8u, 0u }, /* exercism */
  { 0x3897d970u, 10u, 0u }, /* exfatprogs */
  { 0xed832398u, 6u, 0u }, /* exhale */
  { 0x7923f829u, 8u, 0u }, /* exiftool */
  { 0x733be097u, 5u, 0u }, /* exiv2 */
  { 0x96da6b58u, 6u, 0u }, /* expect */
  { 0x6faaff9du, 3u, 0u }, /* eza */
  { 0xbcfbb5f5u, 6u, 0u }, /* fact++ */
  { 0xa5a4c1a4u, 8u, 0u }, /* fakeroot */
  { 0xf293e97fu, 4u, 0u }, /* fasd */
  { 0x5cc79129u, 9u, 0u }, /* fastfetch */
  { 0x0ab0355du, 7u, 0u }, /* fastmod */
  { 0xd1a72fb0u, 7u, 0u }, /* fatsort */
  { 0xbd1dbfd8u, 5u, 0u }, /* faust */
  { 0xd667bca7u, 7u, 0u }, /* fclones */
  { 0xc6fe39dau, 3u, 0u }, /* fcp */
  { 0x6922f347u, 2u, 0u }, /* fd */
  { 0xbc167ddbu, 6u, 0u }, /* fdkaac */
  { 0xac04cb1eu, 3u, 0u }, /* fdm */
  { 0x47b694e2u, 8u, 0u }, /* fdroidcl */
  { 0xff89ab72u, 6u, 0u }, /* fdupes */
  { 0xb3448729u, 6u, 0u }, /* fennel */
  { 0x93aaf8f4u, 11u, 0u }, /* feroxbuster */
  { 0x7d83d2acu, 9u, 0u }, /* fetchmail */
  { 0xb3095351u, 3u, 0u }, /* fff */
  { 0x1b873b98u, 6u, 0u }, /* ffmpeg */
  { 0xad4d8ac3u, 17u, 0u }, /* ffmpegthumbnailer */
  { 0x25389707u, 6u, 0u }, /* ffsend */
  { 0x0dd09044u, 4u, 0u }, /* fftw */
  { 0x01cb83cau, 6u, 0u }, /* figlet */
  { 0xaaea5743u, 4u, 0u }, /* file */
  { 0x3594e4e9u, 5u, 0u }, /* finch */
  { 0xef624f9eu, 9u, 0u }, /* findomain */
  { 0xd7981db7u, 9u, 0u }, /* findutils */
  { 0xafad8963u, 4u, 0u }, /* fish */
  { 0x9851ad91u, 5u, 0u }, /* flang */
  { 0x00afc975u, 11u, 0u }, /* flatbuffers */
  { 0xcae02ff2u, 4u, 0u }, /* flex */
  { 0x0ce8e36bu, 10u, 0u }, /* fluidsynth */
  { 0x5e4e71ebu, 6u, 0u }, /* flyctl */
  { 0x6022e51cu, 2u, 0u }, /* fm */
  { 0xbeef22b8u, 3u, 0u }, /* fmt */
  { 0x7e0bae3eu, 10u, 0u }, /* fontconfig */
  { 0x40c8d4b3u, 7u, 0u }, /* forgejo */
  { 0xe6a5f23eu, 7u, 0u }, /* fortune */
  { 0x53972c15u, 6u, 0u }, /* fossil */
  { 0xc247f8bau, 9u, 0u }, /* freecolor */
  { 0xfc2c3120u, 9u, 0u }, /* freeimage */
  { 0xb490e9c3u, 8u, 0u }, /* freetype */
  { 0x92821340u, 14u, 0u }, /* frei0r-plugins */
  { 0x65de2de7u, 12u, 0u }, /* fresh-editor */
  { 0xdde1957eu, 7u, 0u }, /* fribidi */
  { 0x98762ee6u, 8u, 0u }, /* frobtads */
  { 0xd76ac190u, 5u, 0u }, /* frotz */
  { 0xacd7ad13u, 3u, 0u }, /* frp */
  { 0x274d0f01u, 7u, 0u }, /* fselect */
  { 0x34c23ff4u, 5u, 0u }, /* fsmon */
  { 0xb4ae2958u, 6u, 0u }, /* fwknop */
  { 0x4d22c733u, 2u, 0u }, /* fx */
  { 0xd2c4612du, 3u, 0u }, /* fzf */
  { 0xb3c43060u, 3u, 0u }, /* fzy */
  { 0x33dba4b1u, 14u, 0u }, /* game-music-emu */
  { 0x481721cfu, 3u, 0u }, /* gap */
  { 0xf86ddbe3u, 7u, 0u }, /* gatling */
  { 0x54b72842u, 6u, 0u }, /* gauche */
  { 0x40636bc7u, 4u, 0u }, /* gawk */
  { 0x520ee2c8u, 3u, 0u }, /* gbt */
  { 0x6435ac08u, 4u, 0u }, /* gcab */
  { 0x6235a8e2u, 4u, 0u }, /* gcal */
  { 0xfa24612bu, 4u, 0u }, /* gdal */
  { 0x380a3cacu, 3u, 0u }, /* gdb */
  { 0xf91da3d3u, 4u, 0u }, /* gdbm */
  { 0xd59f4edcu, 10u, 0u }, /* gdk-pixbuf */
  { 0x0cf1a1e2u, 17u, 0u }, /* gdrive-downloader */
  { 0x470a5449u, 3u, 0u }, /* gdu */
  { 0x363ec5ceu, 11u, 0u }, /* geckodriver */
  { 0x58aeb7f0u, 6u, 0u }, /* gecode */
  { 0x89161fc2u, 4u, 0u }, /* gegl */
  { 0x4fa97164u, 9u, 0u }, /* gengetopt */
  { 0x1f36fbfdu, 13u, 0u }, /* geographiclib */
  { 0x71e8cb67u, 15u, 0u }, /* geoip2-database */
  { 0xcb085d24u, 9u, 0u }, /* germanium */
  { 0x2da55339u, 7u, 0u }, /* getconf */
  { 0x8eeb482du, 4u, 0u }, /* geth */
  { 0x6b6fcb70u, 7u, 0u }, /* gettext */
  { 0xafc4c10eu, 6u, 0u }, /* gexiv2 */
  { 0xa3b9e401u, 6u, 0u }, /* gflags */
  { 0x2011e5f9u, 6u, 0u }, /* gforth */
  { 0x47207f2au, 2u, 0u }, /* gh */
  { 0x41285febu, 3u, 0u }, /* ghc */
  { 0xd976a1d3u, 11u, 0u }, /* ghostscript */
  { 0x1d3f1168u, 6u, 0u }, /* giflib */
  { 0xb6a01475u, 8u, 0u }, /* gifsicle */
  { 0x215e0c0au, 6u, 0u }, /* gifski */
  { 0x542abc6bu, 3u, 0u }, /* git */
  { 0x965b7d05u, 22u, 0u }, /* git-credential-manager */
  { 0xc92c324au, 9u, 0u }, /* git-crypt */
  { 0xdd42d3a4u, 9u, 0u }, /* git-delta */
  { 0x675c9e19u, 10u, 0u }, /* git-extras */
  { 0x84f21ba5u, 7u, 0u }, /* git-lfs */
  { 0x4c2cd9ebu, 9u, 0u }, /* git-sizer */
  { 0x03cc8cfau, 8u, 0u }, /* git-town */
  { 0xd0426671u, 5u, 0u }, /* gitea */
  { 0xf3abb3abu, 11u, 0u }, /* gitflow-avh */
  { 0xa60b1a5cu, 8u, 0u }, /* gitoxide */
  { 0xe8699ba9u, 5u, 0u }, /* gitui */
  { 0xf33ac76eu, 7u, 0u }, /* gkermit */
  { 0xae858ee2u, 8u, 0u }, /* glab-cli */
  { 0x27006997u, 5u, 0u }, /* gleam */
  { 0x7f475355u, 4u, 0u }, /* glib */
  { 0x77a3d902u, 15u, 0u }, /* glib-networking */
  { 0xb936d465u, 10u, 0u }, /* glibc-repo */
  { 0x5f1e01c9u, 3u, 0u }, /* glm */
  { 0x1dff06aeu, 6u, 0u }, /* global */
  { 0x6a3827bcu, 4u, 0u }, /* glow */
  { 0x5e2fc613u, 4u, 0u }, /* glpk */
  { 0x07dee1b7u, 7u, 0u }, /* glslang */
  { 0x2d49cd10u, 8u, 0u }, /* gluelang */
  { 0x9a324178u, 6u, 0u }, /* glulxe */
  { 0xd2b15631u, 4u, 0u }, /* gmic */
  { 0x412075b8u, 2u, 0u }, /* gn */
  { 0x55d4deb5u, 6u, 0u }, /* gnucap */
  { 0xba46a911u, 8u, 0u }, /* gnuchess */
  { 0x2deae102u, 8u, 0u }, /* gnucobol */
  { 0xa84c4ecdu, 5u, 0u }, /* gnugo */
  { 0xa16ed60au, 5u, 0u }, /* gnuit */
  { 0x80fa26eau, 6u, 0u }, /* gnunet */
  { 0x80364986u, 5u, 0u }, /* gnupg */
  { 0x8784f548u, 7u, 0u }, /* gnuplot */
  { 0xab321009u, 5u, 0u }, /* gnurl */
  { 0x03bc3769u, 8u, 0u }, /* gnushogi */
  { 0x7afa2124u, 6u, 0u }, /* gnuski */
  { 0xeb515b82u, 12u, 0u }, /* gnustep-make */
  { 0x387aed0fu, 17u, 0u }, /* go-findimagedupes */
  { 0x77be7612u, 11u, 0u }, /* go-musicfox */
  { 0x8fdafae1u, 8u, 0u }, /* goaccess */
  { 0x96b22cd5u, 6u, 0u }, /* gobang */
  { 0xa16cd85fu, 21u, 0u }, /* gobject-introspection */
  { 0x0173c495u, 4u, 0u }, /* gogs */
  { 0xfb57e4a6u, 4u, 0u }, /* gojq */
  { 0x555d0d33u, 6u, 0u }, /* golang */
  { 0xfa64af06u, 4u, 0u }, /* gomp */
  { 0xaf6b3afbu, 6u, 0u }, /* gomuks */
  { 0x736d8848u, 11u, 0u }, /* google-glog */
  { 0xd026326eu, 10u, 0u }, /* googletest */
  { 0xc2a805b8u, 5u, 0u }, /* goose */
  { 0xfe572f8eu, 6u, 0u }, /* gopass */
  { 0xd0ac6e28u, 6u, 0u }, /* gopher */
  { 0x90124fccu, 5u, 0u }, /* gopls */
  { 0x1d935fa1u, 7u, 0u }, /* goresym */
  { 0x0a91e7d4u, 4u, 0u }, /* gost */
  { 0xf4f723e5u, 6u, 0u }, /* gotify */
  { 0x45b18922u, 5u, 0u }, /* gotop */
  { 0xcc4b63e7u, 9u, 0u }, /* gotorrent */
  { 0x46eb7646u, 5u, 0u }, /* gotty */
  { 0xa12b8c8eu, 4u, 0u }, /* gpac */
  { 0x63779229u, 5u, 0u }, /* gperf */
  { 0x8b9af089u, 5u, 0u }, /* gpgme */
  { 0x8154cfd1u, 7u, 0u }, /* gpgmepp */
  { 0x07460d5eu, 5u, 0u }, /* gping */
  { 0xf66c5f55u, 8u, 0u }, /* gpsbabel */
  { 0xa7ce4d7eu, 6u, 0u }, /* gradle */
  { 0x418c136bu, 7u, 0u }, /* grafana */
  { 0xb39aa895u, 4u, 0u }, /* grap */
  { 0xb2f2a867u, 8u, 0u }, /* graphene */
  { 0x7b390260u, 14u, 0u }, /* graphicsmagick */
  { 0x00e3f94cu, 8u, 0u }, /* graphviz */
  { 0x0b1dfc7au, 5u, 0u }, /* greed */
  { 0xbb9027d1u, 4u, 0u }, /* grep */
  { 0xb3901b39u, 4u, 0u }, /* grex */
  { 0x7f98fd41u, 5u, 0u }, /* groff */
  { 0xa5828da5u, 4u, 0u }, /* gron */
  { 0x732edfd8u, 7u, 0u }, /* groonga */
  { 0x4a3ab40fu, 6u, 0u }, /* groovy */
  { 0x0081e07eu, 7u, 0u }, /* grpcurl */
  { 0x4039a779u, 3u, 0u }, /* gsl */
  { 0x84f54fa0u, 9u, 0u }, /* gst-libav */
  { 0x505e11f6u, 15u, 0u }, /* gst-plugins-bad */
  { 0x26f56feeu, 16u, 0u }, /* gst-plugins-base */
  { 0x496f12f2u, 16u, 0u }, /* gst-plugins-good */
  { 0xc5f33cf0u, 16u, 0u }, /* gst-plugins-ugly */
  { 0x893be096u, 10u, 0u }, /* gst-python */
  { 0x20db81a1u, 9u, 0u }, /* gstreamer */
  { 0xafa2a46du, 7u, 0u }, /* gtypist */
  { 0x91dd00d3u, 5u, 0u }, /* guile */
  { 0x3b35226cu, 3u, 0u }, /* gum */
  { 0x0c888d21u, 12u, 0u }, /* gumbo-parser */
  { 0x10767a23u, 18u, 0u }, /* gweather-locations */
  { 0x1a451735u, 4u, 0u }, /* gzip */
  { 0xc14ff7feu, 7u, 0u }, /* haproxy */
  { 0x9b74178fu, 8u, 0u }, /* harfbuzz */
  { 0x0d7f0ebeu, 12u, 0u }, /* hash-slinger */
  { 0xe3f18623u, 8u, 0u }, /* hashdeep */
  { 0x06b92710u, 3u, 0u }, /* hcl */
  { 0xf2238e44u, 6u, 0u }, /* hcloud */
  { 0x62ac1687u, 5u, 0u }, /* helix */
  { 0x4f9f2cabu, 5u, 0u }, /* hello */
  { 0x1d717979u, 4u, 0u }, /* helm */
  { 0xdcbd2c39u, 7u, 0u }, /* helm-ls */
  { 0xb6bfc0ecu, 8u, 0u }, /* help2man */
  { 0x45ac18dcu, 8u, 0u }, /* hexcurse */
  { 0x743d6fe6u, 7u, 0u }, /* hexedit */
  { 0x515a311du, 5u, 0u }, /* hexer */
  { 0x33648f3fu, 5u, 0u }, /* hexyl */
  { 0x3155c278u, 4u, 0u }, /* heyu */
  { 0x915256cdu, 8u, 0u }, /* hfsutils */
  { 0xaaa4ca9eu, 6u, 0u }, /* hidapi */
  { 0x1e0b17a0u, 7u, 0u }, /* hilbish */
  { 0x093401beu, 7u, 0u }, /* hledger */
  { 0x0daa12c7u, 10u, 0u }, /* hledger-ui */
  { 0x0f16e8e3u, 7u, 0u }, /* hoedown */
  { 0x1a1809fau, 9u, 0u }, /* hollywood */
  { 0xc4fc87e7u, 4u, 0u }, /* hors */
  { 0x39e8110eu, 4u, 0u }, /* hstr */
  { 0xe55bb134u, 14u, 0u }, /* html-xml-utils */
  { 0x51adc855u, 9u, 0u }, /* html2text */
  { 0xbf79ff36u, 4u, 0u }, /* htop */
  { 0x842d0d3du, 6u, 0u }, /* htslib */
  { 0x2ded845bu, 7u, 0u }, /* httping */
  { 0xd2f59250u, 7u, 0u }, /* httrack */
  { 0xe48d64ecu, 3u, 0u }, /* hub */
  { 0x6d8eb0d4u, 4u, 0u }, /* hugo */
  { 0x6ea5cc8bu, 6u, 0u }, /* hummin */
  { 0x66f62612u, 8u, 0u }, /* hunspell */
  { 0x2b6d4271u, 14u, 0u }, /* hunspell-en-us */
  { 0xabc01ad9u, 11u, 0u }, /* hunspell-fr */
  { 0xc0b1315eu, 11u, 0u }, /* hunspell-hu */
  { 0xc5ab290fu, 11u, 0u }, /* hunspell-nl */
  { 0xbcf1d268u, 11u, 0u }, /* hunspell-ru */
  { 0xf28d7af6u, 3u, 0u }, /* hut */
  { 0xe2213568u, 6u, 0u }, /* hwdata */
  { 0xbaaecfbdu, 9u, 0u }, /* hydroxide */
  { 0x591a6a59u, 9u, 0u }, /* hyperfine */
  { 0x573adbd7u, 2u, 0u }, /* hz */
  { 0x7c2b5696u, 4u, 0u }, /* i2pd */
  { 0x97dcb591u, 3u, 0u }, /* iat */
  { 0x1ff5976fu, 7u, 0u }, /* icecast */
  { 0xcb27e689u, 4u, 0u }, /* ices */
  { 0x6e7413abu, 17u, 0u }, /* icon-naming-utils */
  { 0x6cf5353bu, 8u, 0u }, /* icoutils */
  { 0x83600560u, 6u, 0u }, /* id3lib */
  { 0xea50f5adu, 5u, 0u }, /* id3v2 */
  { 0xed04cf5eu, 11u, 0u }, /* imagemagick */
  { 0xae2c5d42u, 5u, 0u }, /* imath */
  { 0xa6cdac80u, 6u, 0u }, /* imlib2 */
  { 0x16ff7dbbu, 6u, 0u }, /* indent */
  { 0x31294f08u, 9u, 0u }, /* inetutils */
  { 0xc29df89fu, 8u, 0u }, /* influxdb */
  { 0x00ddba06u, 11u, 0u }, /* innoextract */
  { 0x35279c3bu, 13u, 0u }, /* inotify-tools */
  { 0xe3fb4920u, 8u, 0u }, /* intltool */
  { 0x2d88a9a1u, 4u, 0u }, /* inxi */
  { 0x34e36e61u, 6u, 0u }, /* ipcalc */
  { 0xc345c00cu, 6u, 0u }, /* iperf3 */
  { 0xeb2c2cb8u, 8u, 0u }, /* ipmitool */
  { 0xd394d229u, 8u, 0u }, /* iproute2 */
  { 0x94be9745u, 8u, 0u }, /* ipv6calc */
  { 0x2fce3c36u, 11u, 0u }, /* ipv6toolkit */
  { 0x83f1d086u, 9u, 0u }, /* ircd-irc2 */
  { 0x21a075d3u, 4u, 0u }, /* ired */
  { 0xe589a91bu, 5u, 0u }, /* irssi */
  { 0x7ddf1a87u, 5u, 0u }, /* isync */
  { 0xd952c28au, 8u, 0u }, /* iverilog */
  { 0xe8b475ffu, 4u, 0u }, /* iwyu */
  { 0x2e53cc9au, 4u, 0u }, /* jack */
  { 0x6821f607u, 18u, 0u }, /* jack-example-tools */
  { 0x95eb2c78u, 5u, 0u }, /* jack2 */
  { 0x752cebfdu, 7u, 0u }, /* jackett */
  { 0x134258f8u, 4u, 0u }, /* jadx */
  { 0x988776ffu, 8u, 0u }, /* jbig2dec */
  { 0x3c5a3bb7u, 8u, 0u }, /* jbig2enc */
  { 0xe27da8d7u, 4u, 0u }, /* jcal */
  { 0xf0605cf0u, 15u, 0u }, /* jellyfin-server */
  { 0xcf4713a0u, 9u, 0u }, /* jfrog-cli */
  { 0x4ab475c5u, 5u, 0u }, /* jftui */
  { 0x4b6c1b6bu, 5u, 0u }, /* jhead */
  { 0xb217fa30u, 5u, 0u }, /* jigdo */
  { 0x1a362552u, 7u, 0u }, /* jira-go */
  { 0x53699780u, 5u, 0u }, /* jless */
  { 0x623f6a56u, 2u, 0u }, /* jo */
  { 0xdcd42e49u, 3u, 0u }, /* joe */
  { 0xbdd537dfu, 4u, 0u }, /* jove */
  { 0xd55ec5dau, 4u, 0u }, /* jp2a */
  { 0xa0771f4au, 9u, 0u }, /* jpegoptim */
  { 0x5c3f60e4u, 2u, 0u }, /* jq */
  { 0x508cca36u, 6u, 0u }, /* jq-lsp */
  { 0xbfc4f618u, 3u, 0u }, /* jql */
  { 0x27903bebu, 6u, 0u }, /* json-c */
  { 0x352c896eu, 9u, 0u }, /* json-glib */
  { 0x9db0b7c0u, 7u, 0u }, /* jsoncpp */
  { 0xa73f5c0du, 4u, 0u }, /* jump */
  { 0x991ef258u, 4u, 0u }, /* jupp */
  { 0x8f259e5fu, 4u, 0u }, /* just */
  { 0x1d0466e9u, 6u, 0u }, /* jython */
  { 0x0774e32fu, 8u, 0u }, /* k2pdfopt */
  { 0x6abe29aeu, 3u, 0u }, /* k9s */
  { 0x57c54a5bu, 16u, 0u }, /* kainjow-mustache */
  { 0x4dbaafe3u, 7u, 0u }, /* kakoune */
  { 0x232609b9u, 11u, 0u }, /* kakoune-lsp */
  { 0x7b049d34u, 3u, 0u }, /* kbd */
  { 0x7f6cc479u, 7u, 0u }, /* keybase */
  { 0xafa1de55u, 8u, 0u }, /* keychain */
  { 0xbc1e41f8u, 4u, 0u }, /* kibi */
  { 0xc7d65b25u, 11u, 0u }, /* kiwix-tools */
  { 0x32427a81u, 6u, 0u }, /* knockd */
  { 0x9125bfe6u, 4u, 0u }, /* kona */
  { 0x9d8cf0ecu, 6u, 0u }, /* kotlin */
  { 0x8a6c98b1u, 4u, 0u }, /* krb5 */
  { 0xb50a241fu, 9u, 0u }, /* kubecolor */
  { 0xe84ff609u, 7u, 0u }, /* kubectl */
  { 0x7c998dc5u, 9u, 0u }, /* kubelogin */
  { 0xcabac2d6u, 4u, 0u }, /* kubo */
  { 0x3c508375u, 10u, 0u }, /* ladspa-sdk */
  { 0x551ec567u, 12u, 0u }, /* lastpass-cli */
  { 0x19c68a15u, 7u, 0u }, /* lazygit */
  { 0xa2e089d5u, 4u, 0u }, /* lcal */
  { 0x636d607au, 3u, 0u }, /* ldc */
  { 0x5e6d589bu, 3u, 0u }, /* ldd */
  { 0x8e13eaaeu, 4u, 0u }, /* ldns */
  { 0xee7c1c8cu, 6u, 0u }, /* ledger */
  { 0x2d4d5588u, 4u, 0u }, /* lego */
  { 0x531b843eu, 9u, 0u }, /* leptonica */
  { 0x216b57b8u, 4u, 0u }, /* less */
  { 0xe76ce6a4u, 8u, 0u }, /* lesspipe */
  { 0x8e00641bu, 7u, 0u }, /* leveldb */
  { 0x35392621u, 6u, 0u }, /* lexbor */
  { 0x8bd20271u, 6u, 0u }, /* lexter */
  { 0x4b31ce97u, 2u, 0u }, /* lf */
  { 0x880a1853u, 8u, 0u }, /* lfortran */
  { 0xb3cbbd8bu, 4u, 0u }, /* lftp */
  { 0x2fa9cc45u, 14u, 0u }, /* lgogdownloader */
  { 0x3cbbb6e0u, 5u, 0u }, /* lhasa */
  { 0x6e4313fcu, 6u, 0u }, /* liba52 */
  { 0xc10e8570u, 6u, 0u }, /* libacl */
  { 0xa51d63e6u, 6u, 0u }, /* libaml */
  { 0xa1f24c3fu, 23u, 0u }, /* libandroid-complex-math */
  { 0x963aae4bu, 19u, 0u }, /* libandroid-execinfo */
  { 0x5b98cab0u, 15u, 0u }, /* libandroid-glob */
  { 0xe3191296u, 26u, 0u }, /* libandroid-posix-semaphore */
  { 0xe48b5938u, 18u, 0u }, /* libandroid-selinux */
  { 0x641b62eau, 16u, 0u }, /* libandroid-shmem */
  { 0x01cd5529u, 16u, 0u }, /* libandroid-spawn */
  { 0x93932d06u, 15u, 0u }, /* libandroid-stub */
  { 0x0317b401u, 18u, 0u }, /* libandroid-support */
  { 0x29b03014u, 25u, 0u }, /* libandroid-sysv-semaphore */
  { 0x3b3ac2e5u, 17u, 0u }, /* libandroid-utimes */
  { 0xc70e92e9u, 18u, 0u }, /* libandroid-wordexp */
  { 0x437c9108u, 5u, 0u }, /* libao */
  { 0xa218e1ffu, 6u, 0u }, /* libaom */
  { 0x3b0f8c26u, 15u, 0u }, /* libapt-pkg-perl */
  { 0xf0ed3772u, 10u, 0u }, /* libarchive */
  { 0x4d4ec5c1u, 12u, 0u }, /* libarrow-cpp */
  { 0x3b1b2a30u, 7u, 0u }, /* libasio */
  { 0xbfe7746du, 6u, 0u }, /* libass */
  { 0xec3f85ffu, 9u, 0u }, /* libassuan */
  { 0x54faef0cu, 13u, 0u }, /* libatomic-ops */
  { 0xda4d0078u, 7u, 0u }, /* libavif */
  { 0x37bcbcebu, 14u, 0u }, /* libbcprov-java */
  { 0x2ff362edu, 8u, 0u }, /* libblosc */
  { 0x5b24a50du, 9u, 0u }, /* libblosc2 */
  { 0x8329e21fu, 9u, 0u }, /* libbluray */
  { 0x15e6cd6bu, 7u, 0u }, /* libbs2b */
  { 0x3b83530du, 6u, 0u }, /* libbsd */
  { 0x979dcb44u, 9u, 0u }, /* libbullet */
  { 0xa0472a59u, 7u, 0u }, /* libburn */
  { 0xd36ce906u, 6u, 0u }, /* libbz2 */
  { 0xb9d2204bu, 6u, 0u }, /* libc++ */
  { 0xfc1c98edu, 15u, 0u }, /* libc++utilities */
  { 0xc5884d5fu, 11u, 0u }, /* libc-client */
  { 0x163d49bcu, 7u, 0u }, /* libcaca */
  { 0x430828e0u, 8u, 0u }, /* libcairo */
  { 0xcccf24f6u, 14u, 0u }, /* libcairomm-1.0 */
  { 0xbc19feadu, 15u, 0u }, /* libcairomm-1.16 */
  { 0x864176ccu, 6u, 0u }, /* libcap */
  { 0x3cda9a00u, 9u, 0u }, /* libcap-ng */
  { 0xa647b95au, 6u, 0u }, /* libccd */
  { 0xc3c9483du, 7u, 0u }, /* libcddb */
  { 0x993ad8f0u, 6u, 0u }, /* libcdk */
  { 0x81387491u, 6u, 0u }, /* libcec */
  { 0xa589b142u, 15u, 0u }, /* libceres-solver */
  { 0xa6e3b5b7u, 11u, 0u }, /* libchipmunk */
  { 0x272d978du, 14u, 0u }, /* libchromaprint */
  { 0x71271240u, 6u, 0u }, /* libclc */
  { 0x7e2726b7u, 6u, 0u }, /* libcln */
  { 0xc771a75fu, 7u, 0u }, /* libcoap */
  { 0x824e55c7u, 13u, 0u }, /* libcoinor-cgl */
  { 0x2f8d6028u, 13u, 0u }, /* libcoinor-osi */
  { 0x228258b0u, 15u, 0u }, /* libcoinor-utils */
  { 0x9ace9f2du, 21u, 0u }, /* libcommons-lang3-java */
  { 0xa06270bau, 9u, 0u }, /* libconfig */
  { 0x452c722du, 10u, 0u }, /* libconfuse */
  { 0x00093e31u, 14u, 0u }, /* libcpufeatures */
  { 0x4bfec800u, 8u, 0u }, /* libcroco */
  { 0x5c8b1300u, 8u, 0u }, /* libcrypt */
  { 0xa31007a7u, 6u, 0u }, /* libcue */
  { 0x109df17fu, 8u, 0u }, /* libcunit */
  { 0x9b0d1596u, 7u, 0u }, /* libcurl */
  { 0x1f50e34bu, 7u, 0u }, /* libczmq */
  { 0x2c07e7c0u, 9u, 0u }, /* libdaemon */
  { 0xdadd784du, 7u, 0u }, /* libdart */
  { 0x798186a2u, 8u, 0u }, /* libdav1d */
  { 0x466fc9ceu, 5u, 0u }, /* libdb */
  { 0x0cb1b6eau, 8u, 0u }, /* libde265 */
  { 0x7a94d5efu, 10u, 0u }, /* libdeflate */
  { 0xd91db496u, 8u, 0u }, /* libdevil */
  { 0x5b40e6ceu, 11u, 0u }, /* libdispatch */
  { 0x7e2ed27du, 15u, 0u }, /* libdisplay-info */
  { 0xc158c70bu, 12u, 0u }, /* libdjinterop */
  { 0x1651037bu, 7u, 0u }, /* libdmtx */
  { 0xa4d38e89u, 6u, 0u }, /* libdrm */
  { 0xe27e4e13u, 9u, 0u }, /* libduckdb */
  { 0xe2032094u, 10u, 0u }, /* libduktape */
  { 0x3967f3d5u, 9u, 0u }, /* libdvbcsa */
  { 0x1ffe5668u, 9u, 0u }, /* libdvbpsi */
  { 0xabeef313u, 9u, 0u }, /* libdvdnav */
  { 0x3d6ca4b2u, 10u, 0u }, /* libdvdread */
  { 0x1ac9befau, 7u, 0u }, /* libebml */
  { 0xf8631db9u, 10u, 0u }, /* libebur128 */
  { 0x9cc6d5b8u, 7u, 0u }, /* libedit */
  { 0xe8347dd9u, 6u, 0u }, /* libelf */
  { 0x5e8ff2ceu, 7u, 0u }, /* libenet */
  { 0xaf90233du, 8u, 0u }, /* libepoxy */
  { 0xf9458f44u, 11u, 0u }, /* libesqlite3 */
  { 0xa00fd0cbu, 10u, 0u }, /* libetonyek */
  { 0x3c71f8a7u, 5u, 0u }, /* libev */
  { 0x3fb71884u, 8u, 0u }, /* libevent */
  { 0x3085fd12u, 7u, 0u }, /* libexif */
  { 0x7a2638ceu, 8u, 0u }, /* libexpat */
  { 0x1aff6855u, 7u, 0u }, /* libfann */
  { 0x5f224511u, 6u, 0u }, /* libfcl */
  { 0x6906860du, 10u, 0u }, /* libfdk-aac */
  { 0x56156af3u, 6u, 0u }, /* libffi */
  { 0x104b9706u, 11u, 0u }, /* libfixposix */
  { 0x8e6394c2u, 7u, 0u }, /* libflac */
  { 0xd4cfd9ddu, 8u, 0u }, /* libflann */
  { 0x0f0f1c63u, 11u, 0u }, /* libforestdb */
  { 0xff4d5686u, 9u, 0u }, /* libfreexl */
  { 0x9aca06c4u, 8u, 0u }, /* libftxui */
  { 0x8e6fd4edu, 8u, 0u }, /* libfyaml */
  { 0x377800f6u, 5u, 0u }, /* libgc */
  { 0xf24808bbu, 9u, 0u }, /* libgcrypt */
  { 0x3a7805afu, 5u, 0u }, /* libgd */
  { 0x9acec79cu, 13u, 0u }, /* libgedit-gfls */
  { 0xf0ef0a7bu, 6u, 0u }, /* libgee */
  { 0x503f530eu, 7u, 0u }, /* libgeos */
  { 0x4147cd26u, 10u, 0u }, /* libgeotiff */
  { 0x954814e7u, 7u, 0u }, /* libgf2x */
  { 0x45f8f6e8u, 10u, 0u }, /* libgfshare */
  { 0xb94599d2u, 7u, 0u }, /* libgit2 */
  { 0x12779a73u, 13u, 0u }, /* libglibmm-2.4 */
  { 0xd94d2473u, 14u, 0u }, /* libglibmm-2.68 */
  { 0xfc53d8a9u, 8u, 0u }, /* libglvnd */
  { 0x1c9f2d41u, 8u, 0u }, /* libgmime */
  { 0xd402648cu, 6u, 0u }, /* libgmp */
  { 0xe209365bu, 6u, 0u }, /* libgnt */
  { 0x4d9a6e58u, 15u, 0u }, /* libgnustep-base */
  { 0x9095d4e1u, 9u, 0u }, /* libgnutls */
  { 0xe19e826bu, 12u, 0u }, /* libgpg-error */
  { 0x12082778u, 11u, 0u }, /* libgraphite */
  { 0x039ebe38u, 7u, 0u }, /* libgrpc */
  { 0x6a6121cau, 8u, 0u }, /* libgsasl */
  { 0xc1c1a0e0u, 6u, 0u }, /* libgsf */
  { 0xd9cf8bc0u, 7u, 0u }, /* libgtop */
  { 0xe6c896e4u, 6u, 0u }, /* libgts */
  { 0x91553056u, 7u, 0u }, /* libgxps */
  { 0x4f35850du, 9u, 0u }, /* libhangul */
  { 0x65247b3eu, 7u, 0u }, /* libharu */
  { 0x84404eddu, 7u, 0u }, /* libhdf5 */
  { 0x8d640d20u, 7u, 0u }, /* libheif */
  { 0xb323ea5au, 10u, 0u }, /* libhiredis */
  { 0x26fabcdeu, 10u, 0u }, /* libhtmlcxx */
  { 0xcafb8d00u, 9u, 0u }, /* libhyphen */
  { 0x5446db99u, 7u, 0u }, /* libical */
  { 0x53779383u, 6u, 0u }, /* libice */
  { 0x2833a0d1u, 8u, 0u }, /* libiconv */
  { 0x43777a53u, 6u, 0u }, /* libicu */
  { 0x00b0d5e6u, 9u, 0u }, /* libid3tag */
  { 0x3e7533ddu, 6u, 0u }, /* libidn */
  { 0x4180c13du, 7u, 0u }, /* libidn2 */
  { 0x428dd2e0u, 13u, 0u }, /* libimagequant */
  { 0x6a5f3ddfu, 16u, 0u }, /* libimobiledevice */
  { 0x7ffc16c7u, 21u, 0u }, /* libimobiledevice-glue */
  { 0xd4679340u, 8u, 0u }, /* libimtui */
  { 0xc72f3c62u, 7u, 0u }, /* libinih */
  { 0x2f1335c9u, 12u, 0u }, /* libiniparser */
  { 0xf8a34b7bu, 8u, 0u }, /* libiodbc */
  { 0x24e60e34u, 10u, 0u }, /* libisoburn */
  { 0xb16ae332u, 8u, 0u }, /* libisofs */
  { 0xc230f8fcu, 8u, 0u }, /* libjanet */
  { 0xbb689af1u, 8u, 0u }, /* libjansi */
  { 0xe830733eu, 10u, 0u }, /* libjansson */
  { 0x3cd4e8adu, 9u, 0u }, /* libjasper */
  { 0xef75a22bu, 13u, 0u }, /* libjpeg-turbo */
  { 0xcab270d6u, 6u, 0u }, /* libjxl */
  { 0xa9e4978fu, 12u, 0u }, /* libkeyfinder */
  { 0xbe6d28c8u, 8u, 0u }, /* libkiwix */
  { 0x3f7f5a0au, 7u, 0u }, /* libknot */
  { 0x3aeb6b9eu, 9u, 0u }, /* libkokkos */
  { 0xc6e35a53u, 7u, 0u }, /* libksba */
  { 0xe6835920u, 10u, 0u }, /* liblangtag */
  { 0x0bc2bbe6u, 12u, 0u }, /* liblightning */
  { 0xf12740feu, 13u, 0u }, /* libliquid-dsp */
  { 0xbe1698c1u, 7u, 0u }, /* libllvm */
  { 0x0dd04731u, 7u, 0u }, /* liblmdb */
  { 0x235c0afdu, 5u, 0u }, /* liblo */
  { 0xb08b69b7u, 8u, 0u }, /* liblog4c */
  { 0x2f38b3bfu, 10u, 0u }, /* liblog4cxx */
  { 0x20ea8983u, 6u, 0u }, /* liblqr */
  { 0x40f11ed0u, 7u, 0u }, /* liblrdf */
  { 0x7500d40eu, 6u, 0u }, /* liblz4 */
  { 0xb276cde8u, 7u, 0u }, /* liblzma */
  { 0x1c0047f3u, 6u, 0u }, /* liblzo */
  { 0xa6a48cf5u, 6u, 0u }, /* libmaa */
  { 0xa1a48516u, 6u, 0u }, /* libmad */
  { 0x0cd28d19u, 13u, 0u }, /* libmariadbcpp */
  { 0x7dd0c976u, 8u, 0u }, /* libmatio */
  { 0x2c44629au, 11u, 0u }, /* libmatroska */
  { 0x646f9814u, 12u, 0u }, /* libmaxminddb */
  { 0xba33a5e9u, 9u, 0u }, /* libmcrypt */
  { 0x3e5e7415u, 5u, 0u }, /* libmd */
  { 0x032c3bd7u, 7u, 0u }, /* libmdbx */
  { 0xa5ec0fdcu, 12u, 0u }, /* libmediainfo */
  { 0x8e9cd07fu, 9u, 0u }, /* libmesode */
  { 0x881945b3u, 8u, 0u }, /* libmhash */
  { 0x908493dcu, 13u, 0u }, /* libmicrohttpd */
  { 0xa0fe64eau, 10u, 0u }, /* libminizip */
  { 0x108876e2u, 13u, 0u }, /* libminizip-ng */
  { 0x8bc038f1u, 6u, 0u }, /* libmnl */
  { 0xe7bc1c0cu, 10u, 0u }, /* libmodplug */
  { 0xb790056bu, 12u, 0u }, /* libmosquitto */
  { 0xb47b2797u, 10u, 0u }, /* libmp3lame */
  { 0x9853e04du, 10u, 0u }, /* libmp3splt */
  { 0xac7fc58eu, 6u, 0u }, /* libmpc */
  { 0x79d8b868u, 12u, 0u }, /* libmpdclient */
  { 0x4dc3d681u, 8u, 0u }, /* libmpeg2 */
  { 0x8b2fe7cdu, 7u, 0u }, /* libmpfr */
  { 0x2cb5c5feu, 9u, 0u }, /* libmpg123 */
  { 0x0ff010a0u, 10u, 0u }, /* libmsgpack */
  { 0xcedeef5cu, 14u, 0u }, /* libmsgpack-cxx */
  { 0xf02bddc5u, 11u, 0u }, /* libmuparser */
  { 0x1b52d272u, 10u, 0u }, /* libmypaint */
  { 0x1db0a20cu, 9u, 0u }, /* libnats-c */
  { 0x74b17a64u, 7u, 0u }, /* libneon */
  { 0x0be79cb7u, 6u, 0u }, /* libnet */
  { 0x02b2045eu, 9u, 0u }, /* libnettle */
  { 0x8e9e1b9au, 7u, 0u }, /* libnewt */
  { 0xe8e0a9d9u, 6u, 0u }, /* libnfc */
  { 0xf8e0c309u, 6u, 0u }, /* libnfs */
  { 0xb4ef8a6eu, 8u, 0u }, /* libnftnl */
  { 0xcbcee263u, 10u, 0u }, /* libnghttp2 */
  { 0xcacee0d0u, 10u, 0u }, /* libnghttp3 */
  { 0x757eba3cu, 9u, 0u }, /* libngtcp2 */
  { 0x246089beu, 5u, 0u }, /* libnl */
  { 0x014da772u, 7u, 0u }, /* libnova */
  { 0x8809c53eu, 7u, 0u }, /* libnpth */
  { 0x3bb7d24du, 7u, 0u }, /* libnspr */
  { 0x1114c434u, 6u, 0u }, /* libnss */
  { 0x160c7d4eu, 6u, 0u }, /* libntl */
  { 0x9235e46cu, 8u, 0u }, /* libobjc2 */
  { 0x5bdfb3e6u, 6u, 0u }, /* libode */
  { 0xa384fdcfu, 9u, 0u }, /* libodfgen */
  { 0x89e23ae7u, 6u, 0u }, /* libogg */
  { 0xac224527u, 7u, 0u }, /* liboggz */
  { 0x63cc38c6u, 6u, 0u }, /* libolm */
  { 0xb8a6e6ceu, 11u, 0u }, /* libopenblas */
  { 0x635fd21au, 9u, 0u }, /* libopencc */
  { 0xf8fa55bau, 15u, 0u }, /* libopencore-amr */
  { 0x97ae5346u, 10u, 0u }, /* libopenfec */
  { 0xe7105417u, 10u, 0u }, /* libopenmpt */
  { 0xbc5cf92bu, 7u, 0u }, /* libopus */
  { 0x90449f5du, 10u, 0u }, /* libopusenc */
  { 0x75b2bd36u, 6u, 0u }, /* liborc */
  { 0x2221e626u, 9u, 0u }, /* libosmium */
  { 0xecadb527u, 9u, 0u }, /* libosmpbf */
  { 0x6eb72f5fu, 6u, 0u }, /* libotr */
  { 0x889c1851u, 8u, 0u }, /* libowfat */
  { 0x6374dd4au, 6u, 0u }, /* libp11 */
  { 0x503aea54u, 14u, 0u }, /* libp8-platform */
  { 0xc5e8f540u, 8u, 0u }, /* libpagmo */
  { 0xdd8f3e7bu, 14u, 0u }, /* libpangomm-1.4 */
  { 0x0c04962eu, 15u, 0u }, /* libpangomm-2.48 */
  { 0x1bb63c04u, 9u, 0u }, /* libpano13 */
  { 0xa0601fbeu, 8u, 0u }, /* libpaper */
  { 0x3ee97d04u, 7u, 0u }, /* libpcap */
  { 0x36adbc67u, 11u, 0u }, /* libpcsclite */
  { 0x5d5d7697u, 9u, 0u }, /* libphysfs */
  { 0xdd9732d9u, 9u, 0u }, /* libpinyin */
  { 0x24ebb94au, 11u, 0u }, /* libpipeline */
  { 0x423c4b67u, 9u, 0u }, /* libpixman */
  { 0xa0fc7f70u, 10u, 0u }, /* libplacebo */
  { 0x6b61ea7cu, 8u, 0u }, /* libplist */
  { 0x4f4b9af2u, 8u, 0u }, /* libpluto */
  { 0x0d0a608bu, 6u, 0u }, /* libpng */
  { 0xe16601a4u, 12u, 0u }, /* libpngwriter */
  { 0xd7ad296bu, 7u, 0u }, /* libpoco */
  { 0xe2d20b95u, 7u, 0u }, /* libpopt */
  { 0x39fff11bu, 11u, 0u }, /* libprotobuf */
  { 0x085a40b3u, 13u, 0u }, /* libprotobuf-c */
  { 0xd79377a0u, 12u, 0u }, /* libprotozero */
  { 0x03daf42bu, 6u, 0u }, /* libpsl */
  { 0xe6aee98cu, 10u, 0u }, /* libpugixml */
  { 0xc8e062e5u, 11u, 0u }, /* libqrencode */
  { 0x28d99292u, 10u, 0u }, /* libraptor2 */
  { 0xa4eecc87u, 7u, 0u }, /* libraqm */
  { 0xcad30b80u, 9u, 0u }, /* librasqal */
  { 0x8c8e85abu, 8u, 0u }, /* librav1e */
  { 0xe62dbb42u, 6u, 0u }, /* libraw */
  { 0x3b373b6du, 6u, 0u }, /* libre2 */
  { 0xc0f49772u, 23u, 0u }, /* libregexp-assemble-perl */
  { 0x5cba5a8bu, 17u, 0u }, /* libresolv-wrapper */
  { 0xc702fff2u, 8u, 0u }, /* libretls */
  { 0x3c8d1c62u, 10u, 0u }, /* librevenge */
  { 0x23abba0bu, 7u, 0u }, /* librime */
  { 0x779d4d8eu, 11u, 0u }, /* librinutils */
  { 0xcedf80eeu, 10u, 0u }, /* librnnoise */
  { 0xf54e6998u, 10u, 0u }, /* librocksdb */
  { 0xd61e8200u, 7u, 0u }, /* librsvg */
  { 0xd90e7bd1u, 8u, 0u }, /* librsync */
  { 0x2e67038bu, 9u, 0u }, /* librtmidi */
  { 0x98d7b006u, 9u, 0u }, /* librttopo */
  { 0xd628cb14u, 13u, 0u }, /* libsamplerate */
  { 0x335df8bbu, 7u, 0u }, /* libsasl */
  { 0x205ddad2u, 7u, 0u }, /* libsass */
  { 0x111ed126u, 9u, 0u }, /* libsearpc */
  { 0x7af8a63du, 7u, 0u }, /* libseat */
  { 0x322cef06u, 10u, 0u }, /* libseccomp */
  { 0x74608c6cu, 12u, 0u }, /* libsecp256k1 */
  { 0x736f1f10u, 9u, 0u }, /* libsecret */
  { 0xa576f620u, 7u, 0u }, /* libsfdo */
  { 0x71887271u, 8u, 0u }, /* libshout */
  { 0x22e4bf8du, 13u, 0u }, /* libsigc++-2.0 */
  { 0x9c082846u, 13u, 0u }, /* libsigc++-3.0 */
  { 0xbd3803c3u, 20u, 0u }, /* libsignal-protocol-c */
  { 0x54817e84u, 10u, 0u }, /* libsigsegv */
  { 0x86ec3d95u, 8u, 0u }, /* libsixel */
  { 0x51719154u, 12u, 0u }, /* libskiasharp */
  { 0x4641b52cu, 8u, 0u }, /* libslirp */
  { 0x21a97b20u, 5u, 0u }, /* libsm */
  { 0xaea66643u, 9u, 0u }, /* libsnappy */
  { 0x4bb928b9u, 10u, 0u }, /* libsndfile */
  { 0xadde536fu, 9u, 0u }, /* libsodium */
  { 0x1060c0b2u, 10u, 0u }, /* libsoldout */
  { 0xc7f1719cu, 9u, 0u }, /* libsophia */
  { 0x636f85fau, 13u, 0u }, /* libsoundtouch */
  { 0xe50ef7bbu, 7u, 0u }, /* libsoup */
  { 0x1e8fab18u, 8u, 0u }, /* libsoup3 */
  { 0xe52ace38u, 7u, 0u }, /* libsoxr */
  { 0x1173d780u, 15u, 0u }, /* libspatialindex */
  { 0x99993274u, 13u, 0u }, /* libspatialite */
  { 0xdddab865u, 9u, 0u }, /* libspdlog */
  { 0xe51f27b4u, 10u, 0u }, /* libspectre */
  { 0x510fcd73u, 8u, 0u }, /* libspeex */
  { 0x67557715u, 17u, 0u }, /* libspice-protocol */
  { 0xabc14f32u, 15u, 0u }, /* libspice-server */
  { 0xccf86fe0u, 9u, 0u }, /* libsqlite */
  { 0x641921dbu, 6u, 0u }, /* libsrt */
  { 0x6a16ecb6u, 6u, 0u }, /* libssh */
  { 0x861653ccu, 7u, 0u }, /* libssh2 */
  { 0xb7af99bfu, 10u, 0u }, /* libstemmer */
  { 0x0ba96ea3u, 10u, 0u }, /* libstrophe */
  { 0x0f4103e3u, 11u, 0u }, /* libt3config */
  { 0x7f77568bu, 14u, 0u }, /* libt3highlight */
  { 0xad447b28u, 8u, 0u }, /* libt3key */
  { 0xd0182709u, 11u, 0u }, /* libt3widget */
  { 0xf6adf421u, 11u, 0u }, /* libt3window */
  { 0x49f774bdu, 9u, 0u }, /* libtalloc */
  { 0xca81aeb9u, 8u, 0u }, /* libtasn1 */
  { 0x18ff4d1du, 8u, 0u }, /* libtatsu */
  { 0x7da2e2f4u, 6u, 0u }, /* libtbb */
  { 0x28983d04u, 5u, 0u }, /* libtd */
  { 0x4da8a792u, 6u, 0u }, /* libtdb */
  { 0xc24f872du, 10u, 0u }, /* libtermkey */
  { 0x9feb1e25u, 9u, 0u }, /* libtheora */
  { 0xe79c487du, 12u, 0u }, /* libthread-db */
  { 0x77f7408bu, 7u, 0u }, /* libtiff */
  { 0x3a958e02u, 9u, 0u }, /* libtiledb */
  { 0x7b0accfcu, 7u, 0u }, /* libtins */
  { 0x4df4b007u, 10u, 0u }, /* libtinyxml */
  { 0xed31636fu, 11u, 0u }, /* libtinyxml2 */
  { 0x601c698eu, 8u, 0u }, /* libtirpc */
  { 0x0c5bf5d4u, 9u, 0u }, /* libtllist */
  { 0x12e1315cu, 11u, 0u }, /* libtomcrypt */
  { 0x0b5837b6u, 10u, 0u }, /* libtommath */
  { 0x1d667556u, 7u, 0u }, /* libtool */
  { 0x9aed509au, 10u, 0u }, /* libtorrent */
  { 0xb12222fdu, 20u, 0u }, /* libtorrent-rasterbar */
  { 0x5267af44u, 7u, 0u }, /* libtpms */
  { 0x495ee4e6u, 13u, 0u }, /* libtranscript */
  { 0x0c925aedu, 11u, 0u }, /* libtree-ldd */
  { 0x3c1b7a84u, 9u, 0u }, /* libtsduck */
  { 0x4c6fad70u, 10u, 0u }, /* libtvision */
  { 0x222140dfu, 10u, 0u }, /* libtwolame */
  { 0xedd49ac2u, 10u, 0u }, /* libuber-h3 */
  { 0x9ad891acu, 11u, 0u }, /* libuchardet */
  { 0x04286342u, 11u, 0u }, /* libucontext */
  { 0xe0600a31u, 10u, 0u }, /* libudfread */
  { 0xafed03bdu, 10u, 0u }, /* libunbound */
  { 0x359eae1au, 12u, 0u }, /* libunibilium */
  { 0x8f5e498bu, 12u, 0u }, /* libunistring */
  { 0x31a80feeu, 10u, 0u }, /* libunqlite */
  { 0x908e6077u, 7u, 0u }, /* liburcu */
  { 0xe255f6aeu, 6u, 0u }, /* libusb */
  { 0x6c072c50u, 10u, 0u }, /* libusbmuxd */
  { 0x8e9c9a82u, 11u, 0u }, /* libusbredir */
  { 0x3c9a9b17u, 5u, 0u }, /* libuv */
  { 0x2c4b68a6u, 6u, 0u }, /* libv4l */
  { 0x5f7c8c0eu, 9u, 0u }, /* libvbisam */
  { 0x580ca83fu, 10u, 0u }, /* libvidstab */
  { 0x4dba7047u, 8u, 0u }, /* libvigra */
  { 0xc526bc94u, 7u, 0u }, /* libvips */
  { 0xc5b2f2bau, 8u, 0u }, /* libvisio */
  { 0xfbc38deau, 7u, 0u }, /* libvmaf */
  { 0xf927daf1u, 14u, 0u }, /* libvo-amrwbenc */
  { 0xd17d93a9u, 9u, 0u }, /* libvoikko */
  { 0x5f5bd897u, 9u, 0u }, /* libvorbis */
  { 0x38f3d9a6u, 6u, 0u }, /* libvpx */
  { 0x4c69badcu, 8u, 0u }, /* libvterm */
  { 0x2ce03f0au, 6u, 0u }, /* libvxl */
  { 0xcce3ac4bu, 10u, 0u }, /* libwavpack */
  { 0x68a81dd6u, 10u, 0u }, /* libwayland */
  { 0x8d78438eu, 20u, 0u }, /* libwayland-protocols */
  { 0x80257bceu, 7u, 0u }, /* libwebp */
  { 0xdb18e8deu, 26u, 0u }, /* libwebrtc-audio-processing */
  { 0x5022ee80u, 13u, 0u }, /* libwebsockets */
  { 0xac7fc832u, 10u, 0u }, /* libwolfssl */
  { 0x933ce845u, 6u, 0u }, /* libwpd */
  { 0x903ce38cu, 6u, 0u }, /* libwpg */
  { 0x9c3cf670u, 6u, 0u }, /* libwps */
  { 0xcb0dcd9eu, 7u, 0u }, /* libwren */
  { 0x76b3d32cu, 8u, 0u }, /* libwslay */
  { 0x2c9eff15u, 5u, 0u }, /* libwv */
  { 0xeba0a8b2u, 6u, 0u }, /* libx11 */
  { 0x5dc54840u, 7u, 0u }, /* libx264 */
  { 0x5ec549d3u, 7u, 0u }, /* libx265 */
  { 0xf5b47c93u, 9u, 0u }, /* libxapian */
  { 0xa866b0d6u, 6u, 0u }, /* libxau */
  { 0xab6cc5bdu, 6u, 0u }, /* libxcb */
  { 0x6aa9fe86u, 8u, 0u }, /* libxcfun */
  { 0x1317b79eu, 10u, 0u }, /* libxcursor */
  { 0x712fe824u, 14u, 0u }, /* libxdg-basedir */
  { 0xf0a719ecu, 8u, 0u }, /* libxdmcp */
  { 0x2f9ad608u, 10u, 0u }, /* libxdrfile */
  { 0x179260d7u, 7u, 0u }, /* libxext */
  { 0xf1b880efu, 9u, 0u }, /* libxfixes */
  { 0xb7600caeu, 6u, 0u }, /* libxft */
  { 0x458ddd4fu, 5u, 0u }, /* libxi */
  { 0xa24653b9u, 6u, 0u }, /* libxls */
  { 0x9efdc57cu, 13u, 0u }, /* libxlsxwriter */
  { 0x76b5fa1du, 7u, 0u }, /* libxml2 */
  { 0xc6b6780du, 7u, 0u }, /* libxmlb */
  { 0x22450fdcu, 9u, 0u }, /* libxmlrpc */
  { 0x81dfa2bbu, 9u, 0u }, /* libxrandr */
  { 0xf3c5fcacu, 10u, 0u }, /* libxrender */
  { 0xff13fa1fu, 12u, 0u }, /* libxshmfence */
  { 0xc9707f75u, 7u, 0u }, /* libxslt */
  { 0xba443aeau, 6u, 0u }, /* libxss */
  { 0x388dc8d8u, 5u, 0u }, /* libxt */
  { 0xcb1eeb9fu, 7u, 0u }, /* libxtst */
  { 0x3a8dcbfeu, 5u, 0u }, /* libxv */
  { 0x52bd11f9u, 10u, 0u }, /* libxxf86vm */
  { 0x5838524bu, 7u, 0u }, /* libyaml */
  { 0xe8625cdbu, 11u, 0u }, /* libyaml-cpp */
  { 0x7801f2f1u, 6u, 0u }, /* libzen */
  { 0x950b1af4u, 6u, 0u }, /* libzim */
  { 0x337ad569u, 7u, 0u }, /* libzimg */
  { 0x7a0af073u, 6u, 0u }, /* libzip */
  { 0x999051f9u, 17u, 0u }, /* libzita-convolver */
  { 0x820afd0bu, 6u, 0u }, /* libzix */
  { 0x9115a204u, 6u, 0u }, /* libzmq */
  { 0x22419f72u, 9u, 0u }, /* libzopfli */
  { 0x05a4842au, 12u, 0u }, /* libzxing-cpp */
  { 0x53dc17c5u, 8u, 0u }, /* lighttpd */
  { 0x08e10eb8u, 4u, 0u }, /* lilv */
  { 0x6db04aaau, 8u, 0u }, /* lilypond */
  { 0xe9ca9aeeu, 5u, 0u }, /* links */
  { 0xf2ff012au, 4u, 0u }, /* lipl */
  { 0x404cd5b6u, 3u, 0u }, /* lit */
  { 0xe39c3cd0u, 13u, 0u }, /* litespeedtest */
  { 0x41535926u, 9u, 0u }, /* littlecms */
  { 0x7050ea56u, 9u, 0u }, /* llama-cpp */
  { 0x19f7ae83u, 7u, 0u }, /* llbuild */
  { 0x95a024b5u, 14u, 0u }, /* llvm-mingw-w64 */
  { 0xa2d0c7d3u, 20u, 0u }, /* llvm-mingw-w64-tools */
  { 0x4ee9d224u, 4u, 0u }, /* lnav */
  { 0x6253c701u, 3u, 0u }, /* lnd */
  { 0xbeb399d9u, 8u, 0u }, /* locustdb */
  { 0x14261298u, 7u, 0u }, /* logo-ls */
  { 0x80ae7836u, 9u, 0u }, /* logrotate */
  { 0x153a099eu, 5u, 0u }, /* loksh */
  { 0x47438d31u, 7u, 0u }, /* lowdown */
  { 0x5731e17bu, 2u, 0u }, /* lr */
  { 0x8bd68edau, 5u, 0u }, /* lrzip */
  { 0x7995cb2eu, 5u, 0u }, /* lrzsz */
  { 0x3c82e964u, 3u, 0u }, /* lsd */
  { 0x5bfab071u, 4u, 0u }, /* lsix */
  { 0x46099859u, 4u, 0u }, /* lsof */
  { 0xad5c1c00u, 6u, 0u }, /* ltrace */
  { 0x9da1b7beu, 19u, 0u }, /* lua-language-server */
  { 0xfec33f0cu, 7u, 0u }, /* lua-lgi */
  { 0x9e87c8c0u, 8u, 0u }, /* lua-lpeg */
  { 0xa09513f7u, 5u, 0u }, /* lua51 */
  { 0xa195158au, 5u, 0u }, /* lua52 */
  { 0xa295171du, 5u, 0u }, /* lua53 */
  { 0x9b950c18u, 5u, 0u }, /* lua54 */
  { 0x9c950dabu, 5u, 0u }, /* lua55 */
  { 0x4a1c2454u, 6u, 0u }, /* luajit */
  { 0x12ceacedu, 10u, 0u }, /* luajit-lgi */
  { 0x8c690251u, 8u, 0u }, /* luarocks */
  { 0x4e921044u, 3u, 0u }, /* luv */
  { 0xdcef76d7u, 4u, 0u }, /* luvi */
  { 0x6ff7c299u, 5u, 0u }, /* luvit */
  { 0x44920086u, 3u, 0u }, /* lux */
  { 0x56f59a25u, 7u, 0u }, /* lux-cli */
  { 0x849026afu, 3u, 0u }, /* lv2 */
  { 0x792ac2bfu, 6u, 0u }, /* lychee */
  { 0xc8a81a10u, 4u, 0u }, /* lynx */
  { 0x7ac9d168u, 8u, 0u }, /* lyrebird */
  { 0x6fce09cau, 4u, 0u }, /* lzip */
  { 0xf28b76dau, 5u, 0u }, /* lzlib */
  { 0x5bd2677cu, 4u, 0u }, /* lzop */
  { 0x972e74a4u, 2u, 0u }, /* m4 */
  { 0x375a02ebu, 8u, 0u }, /* macchina */
  { 0xc01dae8eu, 17u, 0u }, /* magic-wormhole-rs */
  { 0xe984a38du, 8u, 0u }, /* mailsync */
  { 0xe9d55febu, 9u, 0u }, /* mailutils */
  { 0xe18c56afu, 4u, 0u }, /* make */
  { 0x517428d6u, 10u, 0u }, /* make-guile */
  { 0xc5c8e8fbu, 6u, 0u }, /* mandoc */
  { 0x7f3313b5u, 6u, 0u }, /* mangal */
  { 0x106ef2cdu, 5u, 0u }, /* manim */
  { 0x67a72445u, 8u, 0u }, /* manpages */
  { 0xb4681eb6u, 9u, 0u }, /* mapserver */
  { 0x0e0e65e3u, 7u, 0u }, /* mariadb */
  { 0x823eb28au, 6u, 0u }, /* marisa */
  { 0x8335324au, 19u, 0u }, /* markdown-flashcards */
  { 0x2d0303fbu, 8u, 0u }, /* marksman */
  { 0x52610476u, 10u, 0u }, /* mathomatic */
  { 0x826fd909u, 10u, 0u }, /* matplotlib */
  { 0x3791e6bdu, 12u, 0u }, /* matterbridge */
  { 0x9fccdabcu, 10u, 0u }, /* matterircd */
  { 0x997731c2u, 7u, 0u }, /* matugen */
  { 0xbd57d7dau, 16u, 0u }, /* mautrix-whatsapp */
  { 0xa6e29488u, 5u, 0u }, /* maven */
  { 0x5975d578u, 6u, 0u }, /* maxcso */
  { 0x15317192u, 6u, 0u }, /* maxima */
  { 0xb32eb396u, 6u, 0u }, /* mazter */
  { 0x4bbf88ecu, 7u, 0u }, /* mbedtls */
  { 0x662e2781u, 2u, 0u }, /* mc */
  { 0x004ee056u, 5u, 0u }, /* mcfly */
  { 0x8e536d79u, 6u, 0u }, /* mdbook */
  { 0xaf274921u, 23u, 0u }, /* mdbook-auto-gen-summary */
  { 0x63ddd090u, 15u, 0u }, /* mdbook-cat-prep */
  { 0x46b7716au, 11u, 0u }, /* mdbook-epub */
  { 0xcdcdaf81u, 15u, 0u }, /* mdbook-graphviz */
  { 0x73d70fadu, 12u, 0u }, /* mdbook-katex */
  { 0x85d6f984u, 16u, 0u }, /* mdbook-linkcheck */
  { 0x553dad6fu, 14u, 0u }, /* mdbook-mermaid */
  { 0x8c3d1546u, 17u, 0u }, /* mdbook-open-on-gh */
  { 0xcf68cb6fu, 13u, 0u }, /* mdbook-pikchr */
  { 0xe998ea7du, 15u, 0u }, /* mdbook-plantuml */
  { 0x02e20230u, 32u, 0u }, /* mdbook-presentation-preprocessor */
  { 0xf0d7888du, 13u, 0u }, /* mdbook-svgbob */
  { 0xe24c44adu, 14u, 0u }, /* mdbook-svgbob2 */
  { 0x6a7968cau, 11u, 0u }, /* mdbook-tera */
  { 0x73612742u, 10u, 0u }, /* mdbook-toc */
  { 0xf42e36adu, 8u, 0u }, /* mdbtools */
  { 0x178e1819u, 4u, 0u }, /* mdds */
  { 0x3683094bu, 9u, 0u }, /* mdns-scan */
  { 0xd1ab286cu, 3u, 0u }, /* mdp */
  { 0xf68b903bu, 11u, 0u }, /* media-types */
  { 0x2c045957u, 9u, 0u }, /* mediainfo */
  { 0xeced7fa6u, 8u, 0u }, /* mediamtx */
  { 0xce21be39u, 7u, 0u }, /* megacmd */
  { 0x7db9ba6au, 9u, 0u }, /* megatools */
  { 0xfb92ab48u, 9u, 0u }, /* memcached */
  { 0x02457f8eu, 7u, 0u }, /* mercury */
  { 0xaa00cce7u, 4u, 0u }, /* mesa */
  { 0xe94d0d53u, 5u, 0u }, /* mfcuk */
  { 0x6a2e2dcdu, 2u, 0u }, /* mg */
  { 0x8404523bu, 5u, 0u }, /* micro */
  { 0x558d2a68u, 10u, 0u }, /* microsocks */
  { 0x2f5bd360u, 6u, 0u }, /* miller */
  { 0x7b8d157fu, 7u, 0u }, /* mimetic */
  { 0xdcb4edd5u, 11u, 0u }, /* minesweeper */
  { 0x8cd27b43u, 7u, 0u }, /* minicom */
  { 0xfaa6a47fu, 8u, 0u }, /* minidlna */
  { 0x444b30adu, 8u, 0u }, /* miniflux */
  { 0xa54f1ba4u, 9u, 0u }, /* minimodem */
  { 0x0c3f81afu, 5u, 0u }, /* minio */
  { 0xc95d946du, 9u, 0u }, /* miniserve */
  { 0xbb5e74e7u, 8u, 0u }, /* minisign */
  { 0xa2776b56u, 9u, 0u }, /* miniupnpc */
  { 0x1ad6e8f4u, 8u, 0u }, /* minizinc */
  { 0xa1189f38u, 9u, 0u }, /* mkbootimg */
  { 0xb2c00e36u, 7u, 0u }, /* mkp224o */
  { 0x42c9a972u, 4u, 0u }, /* mksh */
  { 0xa79d8069u, 9u, 0u }, /* mktorrent */
  { 0xd3aa6ebau, 7u, 0u }, /* mlocate */
  { 0x1d82b4edu, 4u, 0u }, /* mold */
  { 0xf242960fu, 6u, 0u }, /* monero */
  { 0x2e5cbc92u, 7u, 0u }, /* monetdb */
  { 0xf286ee6au, 4u, 0u }, /* mono */
  { 0x0949f745u, 8u, 0u }, /* monolith */
  { 0xcdcdd0a7u, 10u, 0u }, /* moon-buggy */
  { 0x0d84da54u, 4u, 0u }, /* moor */
  { 0xd39e5f9fu, 3u, 0u }, /* mop */
  { 0x06cab57du, 9u, 0u }, /* moreutils */
  { 0x47543fbfu, 5u, 0u }, /* moria */
  { 0x1f742320u, 11u, 0u }, /* morse2ascii */
  { 0x0f5340aeu, 4u, 0u }, /* mosh */
  { 0xfb532132u, 4u, 0u }, /* most */
  { 0x4280e5f3u, 6u, 0u }, /* mp3cat */
  { 0x380b4dd8u, 9u, 0u }, /* mp3cat-go */
  { 0x884ddd3au, 7u, 0u }, /* mp3gain */
  { 0x5891ea1au, 7u, 0u }, /* mp3splt */
  { 0x2b3e41b9u, 7u, 0u }, /* mp3wrap */
  { 0xec79b621u, 3u, 0u }, /* mpc */
  { 0xed79b7b4u, 3u, 0u }, /* mpd */
  { 0x67745fe0u, 11u, 0u }, /* mpdscribble */
  { 0x32a1a94du, 7u, 0u }, /* mplayer */
  { 0xdf79a1aau, 3u, 0u }, /* mpv */
  { 0xc3c52ed0u, 5u, 0u }, /* mruby */
  { 0x4458bde6u, 6u, 0u }, /* ms-gsl */
  { 0x714a35e5u, 6u, 0u }, /* msedit */
  { 0x33dca033u, 8u, 0u }, /* msitools */
  { 0xfc3b5850u, 5u, 0u }, /* msmtp */
  { 0x5e8bdb80u, 9u, 0u }, /* mtd-utils */
  { 0x288ad1a3u, 6u, 0u }, /* mtools */
  { 0x582e1177u, 2u, 0u }, /* mu */
  { 0x9016221fu, 8u, 0u }, /* muchsync */
  { 0xbc471dbcu, 4u, 0u }, /* mujs */
  { 0x92ae32c4u, 9u, 0u }, /* multitail */
  { 0xd495154fu, 5u, 0u }, /* mupdf */
  { 0xda8ca851u, 20u, 0u }, /* music-file-organizer */
  { 0xc70687b7u, 4u, 0u }, /* mutt */
  { 0x2aabfa9du, 5u, 0u }, /* myman */
  { 0x5282eb8eu, 5u, 0u }, /* mympd */
  { 0x3cb617f8u, 15u, 0u }, /* mypaint-brushes */
  { 0xa02d5ad8u, 7u, 0u }, /* myrepos */
  { 0x070213f9u, 11u, 0u }, /* n-t-roff-sc */
  { 0x30d0f275u, 3u, 0u }, /* n2n */
  { 0xad37b1afu, 4u, 0u }, /* nala */
  { 0xb537be47u, 4u, 0u }, /* nali */
  { 0xb33c384fu, 4u, 0u }, /* nano */
  { 0xb0f94f3cu, 4u, 0u }, /* nasm */
  { 0x57195f16u, 7u, 0u }, /* natpmpc */
  { 0xb1000b01u, 4u, 0u }, /* navi */
  { 0xc06d80aeu, 9u, 0u }, /* navidrome */
  { 0x8ab80f0fu, 4u, 0u }, /* ncdc */
  { 0x78b7f2b9u, 4u, 0u }, /* ncdu */
  { 0x9492d0d1u, 5u, 0u }, /* ncdu2 */
  { 0xff75f154u, 5u, 0u }, /* ncftp */
  { 0xef248a80u, 7u, 0u }, /* ncmpcpp */
  { 0x9b38dba1u, 9u, 0u }, /* ncompress */
  { 0xf4878276u, 9u, 0u }, /* ncpamixer */
  { 0x5dd78c26u, 6u, 0u }, /* ncspot */
  { 0xe04dee7eu, 7u, 0u }, /* ncurses */
  { 0x49310e83u, 12u, 0u }, /* ndk-multilib */
  { 0xec697110u, 11u, 0u }, /* ndk-sysroot */
  { 0x5836603cu, 2u, 0u }, /* ne */
  { 0xc11cc99au, 5u, 0u }, /* nelua */
  { 0xae60a2b3u, 11u, 0u }, /* neocmakelsp */
  { 0xba678047u, 8u, 0u }, /* neofetch */
  { 0x4e21354bu, 7u, 0u }, /* neomutt */
  { 0x80159593u, 6u, 0u }, /* neovim */
  { 0xdb51cb7bu, 14u, 0u }, /* neovim-nightly */
  { 0x3c9e049du, 7u, 0u }, /* nerdfix */
  { 0xbe930dcbu, 8u, 0u }, /* net-snmp */
  { 0xfe9526ceu, 9u, 0u }, /* net-tools */
  { 0x47d71238u, 14u, 0u }, /* netcat-openbsd */
  { 0x953d3553u, 8u, 0u }, /* netcdf-c */
  { 0xcba2b571u, 7u, 0u }, /* nethack */
  { 0x1e82e3b9u, 6u, 0u }, /* netpbm */
  { 0x89778108u, 6u, 0u }, /* netsed */
  { 0x7abaf86du, 30u, 0u }, /* netstandard-targeting-pack-2.1 */
  { 0xf98a0174u, 8u, 0u }, /* newsboat */
  { 0x9b73bdc5u, 8u, 0u }, /* newsraft */
  { 0x96e558cfu, 5u, 0u }, /* nginx */
  { 0x4b5d03deu, 6u, 0u }, /* ngircd */
  { 0x516143d4u, 7u, 0u }, /* ngspice */
  { 0x0ea3fa7fu, 3u, 0u }, /* nim */
  { 0xfa8327abu, 5u, 0u }, /* ninja */
  { 0xe34c9103u, 9u, 0u }, /* ninvaders */
  { 0xccae6f07u, 13u, 0u }, /* nlohmann-json */
  { 0x49d834a4u, 5u, 0u }, /* nlopt */
  { 0xcd8a5a4du, 4u, 0u }, /* nmap */
  { 0x31ad2bf4u, 3u, 0u }, /* nmh */
  { 0xaf99359du, 4u, 0u }, /* nmon */
  { 0x11b54859u, 3u, 0u }, /* nnn */
  { 0x5ae1b4e4u, 15u, 0u }, /* no-more-secrets */
  { 0x1f748b62u, 6u, 0u }, /* nodejs */
  { 0xe0730ef2u, 10u, 0u }, /* nodejs-lts */
  { 0xe99527bdu, 9u, 0u }, /* notcurses */
  { 0xfbf54613u, 7u, 0u }, /* notmuch */
  { 0x326a479au, 3u, 0u }, /* npm */
  { 0x3acffa81u, 5u, 0u }, /* npush */
  { 0xba925b2au, 4u, 0u }, /* nsis */
  { 0x36f9f3cfu, 6u, 0u }, /* nsnake */
  { 0xb525935du, 6u, 0u }, /* nudoku */
  { 0x9f47b95eu, 7u, 0u }, /* nushell */
  { 0x0f9dc611u, 7u, 0u }, /* nyancat */
  { 0xd809e28bu, 6u, 0u }, /* nzbget */
  { 0x123f4c5bu, 8u, 0u }, /* oathtool */
  { 0xcd6ca6d8u, 7u, 0u }, /* ocl-icd */
  { 0xefaf462au, 5u, 0u }, /* ocrad */
  { 0x6341e73du, 6u, 0u }, /* octave */
  { 0x0920483cu, 7u, 0u }, /* octomap */
  { 0xab341214u, 7u, 0u }, /* odt2txt */
  { 0xf60d5760u, 10u, 0u }, /* oh-my-posh */
  { 0x86c6aeb7u, 4u, 0u }, /* oidn */
  { 0x5af92691u, 10u, 0u }, /* okc-agents */
  { 0x633432f6u, 2u, 0u }, /* ol */
  { 0x64a83072u, 4u, 0u }, /* oleo */
  { 0x1d9438a3u, 6u, 0u }, /* ollama */
  { 0xa62f4938u, 3u, 0u }, /* oma */
  { 0x78637369u, 8u, 0u }, /* onefetch */
  { 0x827c0d3cu, 6u, 0u }, /* onigmo */
  { 0x6ea58364u, 9u, 0u }, /* oniguruma */
  { 0xab8d340eu, 11u, 0u }, /* onnxruntime */
  { 0x53f17292u, 6u, 0u }, /* oorexx */
  { 0x917b4c60u, 14u, 0u }, /* open-adventure */
  { 0x0681147fu, 11u, 0u }, /* openal-soft */
  { 0x9d9713bfu, 9u, 0u }, /* openbabel */
  { 0x5f9d96f2u, 12u, 0u }, /* opencl-clhpp */
  { 0x2e3eafb5u, 14u, 0u }, /* opencl-headers */
  { 0x772efd00u, 20u, 0u }, /* opencl-vendor-driver */
  { 0xa49a5270u, 11u, 0u }, /* opencolorio */
  { 0xecbb61f2u, 7u, 0u }, /* openexr */
  { 0x580cf968u, 8u, 0u }, /* openfoam */
  { 0xd4f4f9f2u, 6u, 0u }, /* opengl */
  { 0xa677f291u, 8u, 0u }, /* openh264 */
  { 0x267f8fbbu, 10u, 0u }, /* openjdk-17 */
  { 0xa47c847eu, 10u, 0u }, /* openjdk-21 */
  { 0xa87c8acau, 10u, 0u }, /* openjdk-25 */
  { 0x1482c2f7u, 8u, 0u }, /* openjpeg */
  { 0x62f500b0u, 8u, 0u }, /* openldap */
  { 0x4bfcf795u, 8u, 0u }, /* openlist */
  { 0x9d111787u, 7u, 0u }, /* openmpi */
  { 0x86d898a8u, 7u, 0u }, /* openpgl */
  { 0xe0132057u, 6u, 0u }, /* opensc */
  { 0xad9e8f92u, 8u, 0u }, /* openscad */
  { 0xdd44333du, 7u, 0u }, /* openssh */
  { 0xd9442cf1u, 7u, 0u }, /* openssl */
  { 0xc7ca6f92u, 14u, 0u }, /* opentimelineio */
  { 0xd8fd4f03u, 6u, 0u }, /* openxr */
  { 0x00bd8a18u, 7u, 0u }, /* optipng */
  { 0xa085eb0cu, 10u, 0u }, /* opus-tools */
  { 0x80affb1cu, 8u, 0u }, /* opusfile */
  { 0xd872e0e3u, 8u, 0u }, /* opustags */
  { 0x0bf2030au, 7u, 0u }, /* orbiton */
  { 0x0365db1bu, 9u, 0u }, /* osm2pgsql */
  { 0x50313b16u, 9u, 0u }, /* osmctools */
  { 0x646e60f8u, 11u, 0u }, /* osmium-tool */
  { 0x340d1b78u, 12u, 0u }, /* osslsigncode */
  { 0x35dae6b8u, 9u, 0u }, /* ossp-uuid */
  { 0x2de2c40bu, 4u, 0u }, /* ovmf */
  { 0x9ea313c3u, 6u, 0u }, /* oxlint */
  { 0xf0d8c288u, 7u, 0u }, /* p11-kit */
  { 0x84513387u, 5u, 0u }, /* p7zip */
  { 0xe17c4c65u, 6u, 0u }, /* pacman */
  { 0xe96cd278u, 14u, 0u }, /* pacman4console */
  { 0x10b3af74u, 7u, 0u }, /* panda3d */
  { 0x8e113602u, 6u, 0u }, /* pandoc */
  { 0xea02f71au, 5u, 0u }, /* pango */
  { 0xe9405650u, 8u, 0u }, /* paperkey */
  { 0xba7897aeu, 4u, 0u }, /* par2 */
  { 0xd8e0fc32u, 8u, 0u }, /* parallel */
  { 0x61780b93u, 4u, 0u }, /* pari */
  { 0x2e219d85u, 6u, 0u }, /* parted */
  { 0x613ff0e7u, 5u, 0u }, /* paruz */
  { 0x7b7a7318u, 4u, 0u }, /* pass */
  { 0xf5afe984u, 8u, 0u }, /* pass-otp */
  { 0x1a6894e3u, 7u, 0u }, /* passage */
  { 0xd9d4c048u, 14u, 0u }, /* passphrase2pgp */
  { 0x5bccf124u, 10u, 0u }, /* pastebinit */
  { 0x4d7271ccu, 6u, 0u }, /* pastel */
  { 0xf9100aa9u, 5u, 0u }, /* patch */
  { 0x88e0db9au, 8u, 0u }, /* patchelf */
  { 0xfe88814cu, 10u, 0u }, /* pathpicker */
  { 0x4f4e56f7u, 2u, 0u }, /* pb */
  { 0xaeaefbc9u, 4u, 0u }, /* pcal */
  { 0x5fc450ddu, 10u, 0u }, /* pcaudiolib */
  { 0x85d99c45u, 4u, 0u }, /* pcre */
  { 0x2c914f55u, 5u, 0u }, /* pcre2 */
  { 0x07a9b9c1u, 7u, 0u }, /* pdf2svg */
  { 0x7e83f579u, 6u, 0u }, /* pdfcpu */
  { 0x9d438b21u, 7u, 0u }, /* pdfgrep */
  { 0x60a20d08u, 5u, 0u }, /* pdftk */
  { 0xea366ae5u, 8u, 0u }, /* peaclock */
  { 0xb3198498u, 4u, 0u }, /* peco */
  { 0xaff031d8u, 4u, 0u }, /* perl */
  { 0x53744a05u, 11u, 0u }, /* perl-rename */
  { 0x694b8a7eu, 3u, 0u }, /* pet */
  { 0x7a11c2a6u, 6u, 0u }, /* pforth */
  { 0x2cef1e4eu, 8u, 0u }, /* pgroonga */
  { 0xb7035a22u, 10u, 0u }, /* photon-rss */
  { 0x5b44b8afu, 3u, 0u }, /* php */
  { 0xdea5ac43u, 8u, 0u }, /* php-apcu */
  { 0x173e8a2du, 11u, 0u }, /* php-imagick */
  { 0x01c90523u, 7u, 0u }, /* php-psr */
  { 0x0432ae2du, 9u, 0u }, /* php-redis */
  { 0xc46e9bc4u, 17u, 0u }, /* php-zephir-parser */
  { 0x0fe772bcu, 10u, 0u }, /* phpmyadmin */
  { 0x38be3047u, 8u, 0u }, /* pianobar */
  { 0xfa41edf8u, 4u, 0u }, /* pick */
  { 0xfa326a29u, 7u, 0u }, /* picocom */
  { 0xc35653a0u, 8u, 0u }, /* picolisp */
  { 0x134ca2afu, 4u, 0u }, /* pigz */
  { 0x97213934u, 8u, 0u }, /* pikiwidb */
  { 0xc872d738u, 8u, 0u }, /* pinentry */
  { 0x481bd9abu, 6u, 0u }, /* pingme */
  { 0xf354120fu, 10u, 0u }, /* pipebuffer */
  { 0x43a9e2c3u, 8u, 0u }, /* pipes.sh */
  { 0x946331bcu, 8u, 0u }, /* pipewire */
  { 0x908e2e12u, 10u, 0u }, /* pkg-config */
  { 0xf3aa1077u, 7u, 0u }, /* pkgfile */
  { 0x9d4759e0u, 6u, 0u }, /* pkgtop */
  { 0x7112635cu, 8u, 0u }, /* plantuml */
  { 0xdf0cac04u, 10u, 0u }, /* play-audio */
  { 0xc2f09dc0u, 5u, 0u }, /* plzip */
  { 0x1c77920bu, 8u, 0u }, /* pngcrush */
  { 0x96c5ab3fu, 8u, 0u }, /* pngquant */
  { 0x2db8b46au, 10u, 0u }, /* pocketbase */
  { 0x88d2ddfcu, 4u, 0u }, /* poke */
  { 0xbca08a7eu, 6u, 0u }, /* polipo */
  { 0xaafdea76u, 6u, 0u }, /* polyml */
  { 0x3c63a28au, 15u, 0u }, /* pomodoro-curses */
  { 0x6fca0933u, 7u, 0u }, /* poppler */
  { 0x9cba41cau, 12u, 0u }, /* poppler-data */
  { 0x901a04c0u, 9u, 0u }, /* portaudio */
  { 0x5496d0f7u, 8u, 0u }, /* portmidi */
  { 0x6bed8922u, 9u, 0u }, /* posixvala */
  { 0x55c35488u, 7u, 0u }, /* postgis */
  { 0x2ee91e53u, 10u, 0u }, /* postgresql */
  { 0x6933c1e9u, 7u, 0u }, /* potrace */
  { 0x5aab3b2fu, 6u, 0u }, /* pounce */
  { 0xbdac4feeu, 6u, 0u }, /* povray */
  { 0x1c4d962au, 7u, 0u }, /* predict */
  { 0x08500826u, 7u, 0u }, /* privoxy */
  { 0x5a4e7e90u, 6u, 0u }, /* procps */
  { 0x77e4e2d6u, 5u, 0u }, /* procs */
  { 0x82388602u, 18u, 0u }, /* procyon-decompiler */
  { 0xd44ffd93u, 9u, 0u }, /* profanity */
  { 0xcb084e46u, 8u, 0u }, /* progress */
  { 0xe33e21b6u, 4u, 0u }, /* proj */
  { 0x7adbed33u, 5u, 0u }, /* proot */
  { 0x397265a1u, 12u, 0u }, /* proot-distro */
  { 0xbeb81b05u, 13u, 0u }, /* proton-bridge */
  { 0xb962464cu, 7u, 0u }, /* prover9 */
  { 0xedfd6e36u, 9u, 0u }, /* proxmark3 */
  { 0x97a7a813u, 14u, 0u }, /* proxychains-ng */
  { 0x7d37db54u, 6u, 0u }, /* psmisc */
  { 0x0013fb7cu, 4u, 0u }, /* ptex */
  { 0x89bfb6dbu, 10u, 0u }, /* ptunnel-ng */
  { 0x2444b66bu, 5u, 0u }, /* pueue */
  { 0x250f3828u, 10u, 0u }, /* pulseaudio */
  { 0x6d72a03au, 3u, 0u }, /* pup */
  { 0x84c05a8cu, 9u, 0u }, /* pure-ftpd */
  { 0x5b4e69dbu, 2u, 0u }, /* pv */
  { 0x2310e1d2u, 5u, 0u }, /* pwgen */
  { 0x760e7f73u, 8u, 0u }, /* pybind11 */
  { 0x148b97a6u, 7u, 0u }, /* pycairo */
  { 0x8412c7dcu, 9u, 0u }, /* pygobject */
  { 0xe4152ebdu, 4u, 0u }, /* pypy */
  { 0x9b58498au, 5u, 0u }, /* pypy3 */
  { 0xf5074f94u, 7u, 0u }, /* pyrefly */
  { 0x63acae9bu, 8u, 0u }, /* pystring */
  { 0x0fceff97u, 6u, 0u }, /* python */
  { 0xc38834e1u, 11u, 0u }, /* python-apsw */
  { 0x5e9fc6b9u, 10u, 0u }, /* python-apt */
  { 0x75f56e58u, 13u, 0u }, /* python-bcrypt */
  { 0xa1ae71f2u, 13u, 0u }, /* python-brotli */
  { 0x255310c5u, 12u, 0u }, /* python-cmake */
  { 0x120595d7u, 16u, 0u }, /* python-contourpy */
  { 0x901ed5f0u, 19u, 0u }, /* python-cryptography */
  { 0xd3977bfeu, 15u, 0u }, /* python-greenlet */
  { 0x322c6546u, 13u, 0u }, /* python-grpcio */
  { 0xaee0f101u, 14u, 0u }, /* python-lameenc */
  { 0x0eb14bbfu, 14u, 0u }, /* python-libsass */
  { 0x50c2b02bu, 15u, 0u }, /* python-llvmlite */
  { 0x082275fbu, 11u, 0u }, /* python-lxml */
  { 0x0dd6d406u, 14u, 0u }, /* python-msgpack */
  { 0x68750c21u, 12u, 0u }, /* python-numpy */
  { 0x86030adfu, 13u, 0u }, /* python-pillow */
  { 0xca17cb7bu, 10u, 0u }, /* python-pip */
  { 0x808e308fu, 13u, 0u }, /* python-psutil */
  { 0xe4773ad5u, 20u, 0u }, /* python-pycryptodomex */
  { 0xcff6dbc7u, 13u, 0u }, /* python-pynvim */
  { 0x2c63320au, 15u, 0u }, /* python-sabyenc3 */
  { 0xa08281ccu, 12u, 0u }, /* python-scipy */
  { 0x418db54eu, 19u, 0u }, /* python-skia-pathops */
  { 0x6a0c65d5u, 21u, 0u }, /* python-tflite-runtime */
  { 0xa7b78702u, 11u, 0u }, /* python-tldp */
  { 0x3126fba8u, 12u, 0u }, /* python-torch */
  { 0x83c7eb72u, 17u, 0u }, /* python-torchaudio */
  { 0xe5890c8au, 17u, 0u }, /* python-torchcodec */
  { 0xebd6b710u, 18u, 0u }, /* python-torchvision */
  { 0x7f5a7b01u, 11u, 0u }, /* python-xlib */
  { 0x7e661e96u, 13u, 0u }, /* python-yt-dlp */
  { 0x87dc70bfu, 7u, 0u }, /* python2 */
  { 0x97b4ec5au, 12u, 0u }, /* q-dns-client */
  { 0xd4ad4a56u, 4u, 0u }, /* qalc */
  { 0x48d7eaa5u, 27u, 0u }, /* qemu-system-x86-64-headless */
  { 0x1ef9afebu, 5u, 0u }, /* qhull */
  { 0x7b0a6272u, 4u, 0u }, /* qpdf */
  { 0x743c03ccu, 7u, 0u }, /* qrsspig */
  { 0x520137d1u, 11u, 0u }, /* qrupdate-ng */
  { 0x39876d36u, 13u, 0u }, /* quick-lint-js */
  { 0x08ebb371u, 10u, 0u }, /* quickjs-ng */
  { 0xec467e5eu, 5u, 0u }, /* quilt */
  { 0x33860b85u, 15u, 0u }, /* rabbitmq-server */
  { 0x26406953u, 6u, 0u }, /* racket */
  { 0xbf8272b8u, 7u, 0u }, /* radare2 */
  { 0x7eb2af0cu, 4u, 0u }, /* rage */
  { 0xd34a1420u, 5u, 0u }, /* ragel */
  { 0xf2dddee3u, 11u, 0u }, /* railway-cli */
  { 0xcedd3336u, 8u, 0u }, /* range-v3 */
  { 0x8867dfe0u, 6u, 0u }, /* ranger */
  { 0x7c1b3977u, 9u, 0u }, /* rapidjson */
  { 0x9d8381a4u, 4u, 0u }, /* ratt */
  { 0xcb20f214u, 9u, 0u }, /* ravencoin */
  { 0x3405b586u, 3u, 0u }, /* rbw */
  { 0x5e547ec2u, 2u, 0u }, /* rc */
  { 0xb23ab732u, 6u, 0u }, /* rclone */
  { 0x2e036d7du, 3u, 0u }, /* rcm */
  { 0x300370a3u, 3u, 0u }, /* rcs */
  { 0xacb72320u, 6u, 0u }, /* rdfind */
  { 0x198c659fu, 12u, 0u }, /* rdiff-backup */
  { 0xb67426f3u, 6u, 0u }, /* rdircd */
  { 0xd1e31be6u, 7u, 0u }, /* rdrview */
  { 0xd058a53fu, 4u, 0u }, /* re2c */
  { 0x48487c27u, 8u, 0u }, /* readline */
  { 0x5e680da5u, 6u, 0u }, /* recode */
  { 0x857bd2c2u, 6u, 0u }, /* recoll */
  { 0xbfe3d1a2u, 8u, 0u }, /* recutils */
  { 0xb7088b27u, 5u, 0u }, /* redir */
  { 0xb6088994u, 5u, 0u }, /* redis */
  { 0x6404eeb8u, 6u, 0u }, /* remind */
  { 0x23f51b4cu, 11u, 0u }, /* renameutils */
  { 0x3e4a3757u, 6u, 0u }, /* reptyr */
  { 0x458c06d5u, 11u, 0u }, /* resolv-conf */
  { 0x8625453bu, 6u, 0u }, /* restic */
  { 0x9b6a24d5u, 13u, 0u }, /* restic-server */
  { 0x5f84dfd9u, 7u, 0u }, /* restish */
  { 0x5ab54461u, 5u, 0u }, /* rgbds */
  { 0x0cad6d01u, 5u, 0u }, /* rhash */
  { 0x44129aa9u, 3u, 0u }, /* rig */
  { 0x3b951081u, 6u, 0u }, /* rinetd */
  { 0xca249a9au, 4u, 0u }, /* rip2 */
  { 0x162b75acu, 7u, 0u }, /* ripgrep */
  { 0xefb16da6u, 11u, 0u }, /* ripgrep-all */
  { 0x47f4afb9u, 10u, 0u }, /* ripsecrets */
  { 0x03297183u, 4u, 0u }, /* rirc */
  { 0x6dc666d5u, 5u, 0u }, /* rizin */
  { 0xd4f31e2bu, 6u, 0u }, /* rlwrap */
  { 0x3e9b69b1u, 4u, 0u }, /* rmpc */
  { 0x31103e29u, 3u, 0u }, /* rnr */
  { 0xc0b4160cu, 9u, 0u }, /* robin-map */
  { 0x1b3d6ffeu, 16u, 0u }, /* robotfindskitten */
  { 0xbf92ef64u, 9u, 0u }, /* root-repo */
  { 0xf1d75bf6u, 8u, 0u }, /* rp-pppoe */
  { 0x4dd8bed6u, 3u, 0u }, /* rpm */
  { 0x4c54626cu, 2u, 0u }, /* rq */
  { 0x405db455u, 6u, 0u }, /* rsgain */
  { 0xdb2a865du, 9u, 0u }, /* rsnapshot */
  { 0x2553c66au, 5u, 0u }, /* rsync */
  { 0x28575fdcu, 8u, 0u }, /* rtmpdump */
  { 0x1c83eff5u, 8u, 0u }, /* rtorrent */
  { 0x6670e84cu, 10u, 0u }, /* rubberband */
  { 0x729966f7u, 11u, 0u }, /* rubiks-cube */
  { 0x1c14db4du, 4u, 0u }, /* ruby */
  { 0x1d1f6a3cu, 4u, 0u }, /* ruff */
  { 0x88f57317u, 5u, 0u }, /* runit */
  { 0x013f91d3u, 4u, 0u }, /* rush */
  { 0x1d3fbde7u, 4u, 0u }, /* rust */
  { 0xc9e09744u, 13u, 0u }, /* rust-analyzer */
  { 0x14dd9e6du, 12u, 0u }, /* rust-bindgen */
  { 0x9c31f01eu, 8u, 0u }, /* rustscan */
  { 0xd584ca31u, 7u, 0u }, /* rxfetch */
  { 0xf2fa3f73u, 7u, 0u }, /* sabnzbd */
  { 0x2465d7a1u, 5u, 0u }, /* samba */
  { 0xabca1ad5u, 8u, 0u }, /* samefile */
  { 0xdbef1585u, 7u, 0u }, /* samurai */
  { 0xda70ef1cu, 5u, 0u }, /* sassc */
  { 0x800a378fu, 4u, 0u }, /* sbcl */
  { 0x57006c8cu, 5u, 0u }, /* sc-im */
  { 0x869722bdu, 5u, 0u }, /* scala */
  { 0xd31810f0u, 3u, 0u }, /* scc */
  { 0xd434d047u, 7u, 0u }, /* sccache */
  { 0xa33e1d77u, 5u, 0u }, /* scdoc */
  { 0x55c54c11u, 6u, 0u }, /* screen */
  { 0x0f87092fu, 11u, 0u }, /* screenfetch */
  { 0x4edd1710u, 5u, 0u }, /* scrub */
  { 0x52c0effau, 6u, 0u }, /* scrypt */
  { 0x3b520912u, 2u, 0u }, /* sd */
  { 0xf8073e8fu, 4u, 0u }, /* sdcv */
  { 0x6ad61a6eu, 14u, 0u }, /* seafile-client */
  { 0x8f6bbecfu, 7u, 0u }, /* seanime */
  { 0x8e77502fu, 7u, 0u }, /* seccure */
  { 0xc26b7af4u, 13u, 0u }, /* secure-delete */
  { 0xb626edd3u, 3u, 0u }, /* sed */
  { 0x7e4b2fb9u, 6u, 0u }, /* sendme */
  { 0x53e8c208u, 8u, 0u }, /* sendxmpp */
  { 0x28642bedu, 6u, 0u }, /* senpai */
  { 0x0226a1b6u, 14u, 0u }, /* sensible-utils */
  { 0x727e4ac3u, 4u, 0u }, /* serd */
  { 0x747e4de9u, 4u, 0u }, /* serf */
  { 0x6252fa6au, 5u, 0u }, /* sfeed */
  { 0x24e7f4b6u, 6u, 0u }, /* sftpgo */
  { 0x0cece051u, 7u, 0u }, /* shaderc */
  { 0x27c78be6u, 9u, 0u }, /* sharutils */
  { 0xeb2dfd07u, 3u, 0u }, /* shc */
  { 0x259f5e14u, 7u, 0u }, /* sheldon */
  { 0x6c27e7e9u, 10u, 0u }, /* shell2http */
  { 0x19d8e8a3u, 10u, 0u }, /* shellcheck */
  { 0x57646963u, 11u, 0u }, /* shellharden */
  { 0x55820a56u, 11u, 0u }, /* shellinabox */
  { 0xc446f2f1u, 5u, 0u }, /* shfmt */
  { 0x82bb21e7u, 6u, 0u }, /* shiori */
  { 0x8234630au, 7u, 0u }, /* shntool */
  { 0x3f5a4e98u, 6u, 0u }, /* shtool */
  { 0x246c760eu, 7u, 0u }, /* signify */
  { 0x6d659926u, 7u, 0u }, /* silicon */
  { 0x1c81b08au, 17u, 0u }, /* silversearcher-ag */
  { 0x7bb3c4efu, 5u, 0u }, /* simde */
  { 0xa993b362u, 8u, 0u }, /* simdjson */
  { 0x2ecbcbb4u, 4u, 0u }, /* simh */
  { 0x143c86ccu, 8u, 0u }, /* simulavr */
  { 0x88ceb526u, 8u, 0u }, /* sing-box */
  { 0x3a0ffee1u, 5u, 0u }, /* skate */
  { 0x435215aau, 2u, 0u }, /* sl */
  { 0xcebb50beu, 5u, 0u }, /* slang */
  { 0xe7e4524bu, 9u, 0u }, /* slashtime */
  { 0x83b704b8u, 9u, 0u }, /* sleuthkit */
  { 0x6fac698fu, 6u, 0u }, /* slides */
  { 0x30e7af50u, 7u, 0u }, /* slugify */
  { 0x9ba602eau, 9u, 0u }, /* smalltalk */
  { 0x8edd3736u, 15u, 0u }, /* snapcast-server */
  { 0xcdffb6a1u, 6u, 0u }, /* snmptt */
  { 0x91fe08e7u, 9u, 0u }, /* snowflake */
  { 0x9d815c49u, 5u, 0u }, /* socat */
  { 0x8b74ff1eu, 13u, 0u }, /* softether-vpn */
  { 0xf367d836u, 4u, 0u }, /* soju */
  { 0xe21afa70u, 8u, 0u }, /* solidity */
  { 0xf531c162u, 4u, 0u }, /* sops */
  { 0x142be201u, 4u, 0u }, /* sord */
  { 0x3ac04fb7u, 23u, 0u }, /* sound-theme-freedesktop */
  { 0xd6362abdu, 3u, 0u }, /* sox */
  { 0x099606d7u, 16u, 0u }, /* spatialite-tools */
  { 0x1b360609u, 7u, 0u }, /* speechd */
  { 0x75073471u, 12u, 0u }, /* speedtest-go */
  { 0x788940cbu, 8u, 0u }, /* speexdsp */
  { 0x1f1db8bau, 6u, 0u }, /* spglib */
  { 0x58cbceb1u, 12u, 0u }, /* spidermonkey */
  { 0xd10a0c86u, 6u, 0u }, /* spiped */
  { 0x6f43a2f6u, 13u, 0u }, /* spirv-headers */
  { 0x2d6aa102u, 21u, 0u }, /* spirv-llvm-translator */
  { 0x447e8537u, 11u, 0u }, /* spirv-tools */
  { 0xbc42069cu, 9u, 0u }, /* sqlcipher */
  { 0x1f828373u, 17u, 0u }, /* squashfs-tools-ng */
  { 0x947b58a3u, 11u, 0u }, /* squeezelite */
  { 0x45d2fa39u, 5u, 0u }, /* squid */
  { 0x35ed7239u, 6u, 0u }, /* sratom */
  { 0x179e0363u, 6u, 0u }, /* srelay */
  { 0xbb8dee79u, 10u, 0u }, /* srt2vobsub */
  { 0x98229d25u, 4u, 0u }, /* ssdb */
  { 0xb36d5e3du, 6u, 0u }, /* ssdeep */
  { 0x9dfcfbc8u, 8u, 0u }, /* sse2neon */
  { 0x3dc0c724u, 7u, 0u }, /* sshpass */
  { 0x805fc425u, 7u, 0u }, /* sshping */
  { 0x168c6770u, 7u, 0u }, /* sslscan */
  { 0x3a8f1461u, 7u, 0u }, /* sssnake */
  { 0x9f386e79u, 4u, 0u }, /* ssss */
  { 0xe43b4d5au, 4u, 0u }, /* stag */
  { 0x838378cbu, 8u, 0u }, /* starship */
  { 0x0a915e7eu, 6u, 0u }, /* stdman */
  { 0xec7ddf38u, 12u, 0u }, /* stdoutisatty */
  { 0x8fd53986u, 8u, 0u }, /* steghide */
  { 0x015c5980u, 8u, 0u }, /* step-cli */
  { 0xb93d4840u, 4u, 0u }, /* stfl */
  { 0x9ff70753u, 9u, 0u }, /* stockfish */
  { 0xbb4ef5d7u, 6u, 0u }, /* stoken */
  { 0xc81958fau, 5u, 0u }, /* stone */
  { 0x68550fffu, 12u, 0u }, /* storj-uplink */
  { 0xc4531fe0u, 4u, 0u }, /* stow */
  { 0x9e934cffu, 6u, 0u }, /* strace */
  { 0xb4609203u, 12u, 0u }, /* streamripper */
  { 0x5fa8240cu, 7u, 0u }, /* stunnel */
  { 0xf02fd911u, 8u, 0u }, /* stuntman */
  { 0xa7303845u, 6u, 0u }, /* stylua */
  { 0x64327b1fu, 14u, 0u }, /* subtitleripper */
  { 0xbdab29e7u, 10u, 0u }, /* subversion */
  { 0x29153e24u, 4u, 0u }, /* sudo */
  { 0x462faf48u, 4u, 0u }, /* suil */
  { 0xc849422du, 9u, 0u }, /* suite3270 */
  { 0x9427cc43u, 11u, 0u }, /* suitesparse */
  { 0xe04e3f61u, 3u, 0u }, /* sun */
  { 0x5c518014u, 8u, 0u }, /* sundials */
  { 0xca16ddc0u, 13u, 0u }, /* supercollider */
  { 0x83991c65u, 7u, 0u }, /* surfraw */
  { 0x6c27b0ffu, 7u, 0u }, /* svt-av1 */
  { 0x06ad5680u, 5u, 0u }, /* swaks */
  { 0xf39d9762u, 10u, 0u }, /* swi-prolog */
  { 0xb13260d8u, 5u, 0u }, /* swift */
  { 0x472bd8e9u, 11u, 0u }, /* swiftshader */
  { 0x67da044fu, 4u, 0u }, /* swig */
  { 0x8ca35e4au, 5u, 0u }, /* swtpm */
  { 0x4e0e1daau, 9u, 0u }, /* syncthing */
  { 0x02637e57u, 7u, 0u }, /* sysprop */
  { 0x536c4718u, 6u, 0u }, /* ta-lib */
  { 0x89dc366au, 6u, 0u }, /* taglib */
  { 0xfdafa03fu, 5u, 0u }, /* taplo */
  { 0xa8f7477cu, 3u, 0u }, /* tar */
  { 0x14bdf3a9u, 12u, 0u }, /* task-spooler */
  { 0x44dc5bbfu, 6u, 0u }, /* tasksh */
  { 0x27c64bc6u, 11u, 0u }, /* taskwarrior */
  { 0x91f2a619u, 3u, 0u }, /* tcc */
  { 0x9af2b444u, 3u, 0u }, /* tcl */
  { 0x380349f3u, 6u, 0u }, /* tcllib */
  { 0x1222e0f3u, 4u, 0u }, /* tcsh */
  { 0x9d04008bu, 3u, 0u }, /* tdl */
  { 0xb401e629u, 3u, 0u }, /* tea */
  { 0xdfaa12bbu, 8u, 0u }, /* tealdeer */
  { 0x223e0255u, 6u, 0u }, /* teckit */
  { 0x7a30fa58u, 8u, 0u }, /* tectonic */
  { 0x6673533fu, 16u, 0u }, /* telegram-bot-api */
  { 0xa2f1debcu, 12u, 0u }, /* teleport-tsh */
  { 0x9ace123fu, 4u, 0u }, /* tere */
  { 0x702011fau, 7u, 0u }, /* tergent */
  { 0xf07d8f4cu, 9u, 0u }, /* termimage */
  { 0x8652ca85u, 8u, 0u }, /* termplay */
  { 0x18bc17a3u, 9u, 0u }, /* termux-am */
  { 0x13861ab3u, 16u, 0u }, /* termux-am-socket */
  { 0xbe0cad57u, 10u, 0u }, /* termux-api */
  { 0x5528b2ffu, 15u, 0u }, /* termux-apt-repo */
  { 0x0398995fu, 11u, 0u }, /* termux-auth */
  { 0xd4f598eeu, 11u, 0u }, /* termux-core */
  { 0xa6e64670u, 21u, 0u }, /* termux-create-package */
  { 0x93584e1du, 18u, 0u }, /* termux-elf-cleaner */
  { 0x3467384cu, 11u, 0u }, /* termux-exec */
  { 0x9f6fdbd9u, 15u, 0u }, /* termux-gui-bash */
  { 0xf4a4b00cu, 12u, 0u }, /* termux-gui-c */
  { 0x8713e51du, 18u, 0u }, /* termux-gui-package */
  { 0xa06fa648u, 13u, 0u }, /* termux-gui-pm */
  { 0x268fed76u, 14u, 0u }, /* termux-keyring */
  { 0xfb8aeda5u, 15u, 0u }, /* termux-licenses */
  { 0x604800c5u, 15u, 0u }, /* termux-services */
  { 0x584dfd0cu, 12u, 0u }, /* termux-tools */
  { 0x55492989u, 5u, 0u }, /* teseq */
  { 0x0c7a18c5u, 9u, 0u }, /* tesseract */
  { 0xd4e72872u, 10u, 0u }, /* testssl.sh */
  { 0xc06fa84cu, 8u, 0u }, /* tex-gyre */
  { 0x2c1f193eu, 7u, 0u }, /* texinfo */
  { 0x34250555u, 6u, 0u }, /* texlab */
  { 0x91ccd670u, 11u, 0u }, /* texlive-bin */
  { 0x2ed73b55u, 17u, 0u }, /* texlive-installer */
  { 0xd0f3c540u, 4u, 0u }, /* tgpt */
  { 0x87198e02u, 6u, 0u }, /* thrift */
  { 0x6d4dfe35u, 4u, 0u }, /* tidy */
  { 0xc60afcdbu, 3u, 0u }, /* tig */
  { 0xff1023a9u, 5u, 0u }, /* tilde */
  { 0x5d3c9be4u, 4u, 0u }, /* time */
  { 0x037b732eu, 11u, 0u }, /* timewarrior */
  { 0x5f3c9f0au, 4u, 0u }, /* timg */
  { 0x86579b86u, 10u, 0u }, /* timidity++ */
  { 0x64973a3eu, 10u, 0u }, /* tin-summer */
  { 0x930494e9u, 8u, 0u }, /* tintin++ */
  { 0xd4033e15u, 9u, 0u }, /* tinyfugue */
  { 0xc9d486d1u, 6u, 0u }, /* tinygo */
  { 0x13e823a6u, 8u, 0u }, /* tinymist */
  { 0xe3d9b8e5u, 9u, 0u }, /* tinyproxy */
  { 0x963dd7d0u, 10u, 0u }, /* tinyscheme */
  { 0x67474276u, 10u, 0u }, /* tinysparql */
  { 0x2140fe8fu, 7u, 0u }, /* tizonia */
  { 0x3e4541d8u, 2u, 0u }, /* tk */
  { 0xb9b0360eu, 5u, 0u }, /* tmate */
  { 0x1ec621cfu, 4u, 0u }, /* tmux */
  { 0xa6ca5e08u, 6u, 0u }, /* toilet */
  { 0x8f79e253u, 5u, 0u }, /* tokei */
  { 0xda8708a8u, 5u, 0u }, /* tome2 */
  { 0x29464e97u, 6u, 0u }, /* toml11 */
  { 0xc6461b63u, 8u, 0u }, /* topgrade */
  { 0xa910df62u, 3u, 0u }, /* tor */
  { 0xcfdbbbfbu, 8u, 0u }, /* torsocks */
  { 0x3d131867u, 15u, 0u }, /* totem-pl-parser */
  { 0x195419ecu, 5u, 0u }, /* toxic */
  { 0x50c18a45u, 9u, 0u }, /* tracepath */
  { 0xf42fdbc7u, 10u, 0u }, /* traceroute */
  { 0xcff6f4bau, 15u, 0u }, /* translate-shell */
  { 0x079280e5u, 12u, 0u }, /* transmission */
  { 0x6d8b34d5u, 4u, 0u }, /* tree */
  { 0xea8b40c7u, 11u, 0u }, /* tree-sitter */
  { 0xf54accd4u, 16u, 0u }, /* tree-sitter-bash */
  { 0xace0f007u, 13u, 0u }, /* tree-sitter-c */
  { 0x86c26a3du, 15u, 0u }, /* tree-sitter-css */
  { 0xae1018bcu, 14u, 0u }, /* tree-sitter-go */
  { 0x5728fbc3u, 16u, 0u }, /* tree-sitter-html */
  { 0x328d6026u, 16u, 0u }, /* tree-sitter-java */
  { 0xafedf325u, 22u, 0u }, /* tree-sitter-javascript */
  { 0x64e02228u, 16u, 0u }, /* tree-sitter-json */
  { 0xbe25c14eu, 17u, 0u }, /* tree-sitter-latex */
  { 0xdde9b348u, 15u, 0u }, /* tree-sitter-lua */
  { 0x37aeffdbu, 20u, 0u }, /* tree-sitter-markdown */
  { 0x3e70836eu, 19u, 0u }, /* tree-sitter-parsers */
  { 0x8b15cc08u, 18u, 0u }, /* tree-sitter-python */
  { 0x91ae7f22u, 17u, 0u }, /* tree-sitter-query */
  { 0x5d7b0e99u, 17u, 0u }, /* tree-sitter-regex */
  { 0x8a6492ccu, 16u, 0u }, /* tree-sitter-rust */
  { 0xbbcf069au, 15u, 0u }, /* tree-sitter-sql */
  { 0x74b69612u, 16u, 0u }, /* tree-sitter-toml */
  { 0x7bc7b26au, 15u, 0u }, /* tree-sitter-vim */
  { 0x6175db94u, 18u, 0u }, /* tree-sitter-vimdoc */
  { 0x9fcab6d3u, 15u, 0u }, /* tree-sitter-xml */
  { 0xd662c84du, 16u, 0u }, /* tree-sitter-yaml */
  { 0x5818a8beu, 9u, 0u }, /* trojan-go */
  { 0xdd5e6e7du, 5u, 0u }, /* trunk */
  { 0xf6692334u, 5u, 0u }, /* trurl */
  { 0x3658ad37u, 8u, 0u }, /* trzsz-go */
  { 0x4a9c5665u, 9u, 0u }, /* trzsz-ssh */
  { 0xee345f20u, 6u, 0u }, /* tsocks */
  { 0xa419d1dfu, 3u, 0u }, /* tsu */
  { 0x2271766bu, 10u, 0u }, /* ttf-dejavu */
  { 0xb0dff5feu, 18u, 0u }, /* ttf-jetbrains-mono */
  { 0x0b5bac80u, 22u, 0u }, /* ttf-nerd-fonts-symbols */
  { 0x2e7670efu, 9u, 0u }, /* tty-clock */
  { 0xd4bd8375u, 13u, 0u }, /* tty-solitaire */
  { 0xdaf8ab85u, 4u, 0u }, /* ttyc */
  { 0xd3f8a080u, 4u, 0u }, /* ttyd */
  { 0xb387310bu, 6u, 0u }, /* ttyper */
  { 0x45a6f27bu, 7u, 0u }, /* ttyplot */
  { 0x75a5bd52u, 6u, 0u }, /* ttyrec */
  { 0x92323c27u, 4u, 0u }, /* tudo */
  { 0xf7b59e1fu, 8u, 0u }, /* tur-repo */
  { 0xcf3f34fbu, 5u, 0u }, /* turbo */
  { 0xb93d7846u, 9u, 0u }, /* turbopack */
  { 0xa728e122u, 3u, 0u }, /* tut */
  { 0x640167bau, 9u, 0u }, /* tvheadend */
  { 0xc9b81a38u, 6u, 0u }, /* tweego */
  { 0x99e96bd7u, 7u, 0u }, /* txikijs */
  { 0xfeaaa90du, 5u, 0u }, /* typst */
  { 0xbdc54620u, 8u, 0u }, /* typstfmt */
  { 0xf6665fc1u, 8u, 0u }, /* udftools */
  { 0xec6f1524u, 7u, 0u }, /* udocker */
  { 0x3dee623fu, 7u, 0u }, /* uftrace */
  { 0xabb77a50u, 5u, 0u }, /* ugrep */
  { 0xd6c88569u, 4u, 0u }, /* unar */
  { 0xef342d4au, 12u, 0u }, /* unicode-cldr */
  { 0xc269127fu, 12u, 0u }, /* unicode-data */
  { 0xae401cebu, 13u, 0u }, /* unicode-emoji */
  { 0x11439fd7u, 7u, 0u }, /* unicorn */
  { 0x01bac534u, 7u, 0u }, /* unifdef */
  { 0xc918a49cu, 5u, 0u }, /* units */
  { 0x01d3097fu, 8u, 0u }, /* unixodbc */
  { 0x91ab39fcu, 7u, 0u }, /* unpaper */
  { 0x2360460bu, 5u, 0u }, /* unrar */
  { 0x02be6e27u, 8u, 0u }, /* unshield */
  { 0xbe3f1941u, 5u, 0u }, /* unzip */
  { 0x43430b20u, 2u, 0u }, /* up */
  { 0x11c14029u, 6u, 0u }, /* upower */
  { 0x3a8adb88u, 3u, 0u }, /* upx */
  { 0xad03f3dcu, 7u, 0u }, /* urdfdom */
  { 0x88b63293u, 15u, 0u }, /* urdfdom-headers */
  { 0xf3dd625fu, 7u, 0u }, /* usbmuxd */
  { 0x7246de7cu, 4u, 0u }, /* usql */
  { 0xec1645bbu, 7u, 0u }, /* utf8cpp */
  { 0xd6b8597eu, 8u, 0u }, /* utf8proc */
  { 0x50e62a0cu, 6u, 0u }, /* uthash */
  { 0xdcafb8f2u, 10u, 0u }, /* util-linux */
  { 0x379977ecu, 4u, 0u }, /* uucp */
  { 0x49431492u, 2u, 0u }, /* uv */
  { 0x97a2f482u, 5u, 0u }, /* uwsgi */
  { 0x6f4d9673u, 5u, 0u }, /* v2ray */
  { 0x34905a8cu, 5u, 0u }, /* valac */
  { 0x50c3fc5bu, 4u, 0u }, /* vale */
  { 0xc19d8d6au, 8u, 0u }, /* valgrind */
  { 0x1ed0bfabu, 6u, 0u }, /* valkey */
  { 0x28f73022u, 15u, 0u }, /* vamp-plugin-sdk */
  { 0x4e1c5e98u, 11u, 0u }, /* vapoursynth */
  { 0x38791469u, 8u, 0u }, /* vbindiff */
  { 0x2d4e39edu, 4u, 0u }, /* vcsh */
  { 0x16e1e9ccu, 4u, 0u }, /* vde2 */
  { 0x6719b7f3u, 6u, 0u }, /* vegeta */
  { 0xd5351d59u, 4u, 0u }, /* vera */
  { 0x34596c25u, 9u, 0u }, /* vgmstream */
  { 0x16ecab1cu, 8u, 0u }, /* vgmtools */
  { 0xd8f54993u, 5u, 0u }, /* viddy */
  { 0xe5a7434du, 4u, 0u }, /* vifm */
  { 0xb9b60893u, 4u, 0u }, /* vile */
  { 0x75123ef7u, 3u, 0u }, /* vim */
  { 0x0ebd8466u, 13u, 0u }, /* virglrenderer */
  { 0x093d2910u, 21u, 0u }, /* virglrenderer-android */
  { 0xabb8022bu, 14u, 0u }, /* virustotal-cli */
  { 0x7b124869u, 3u, 0u }, /* vis */
  { 0xdc42b9bdu, 8u, 0u }, /* vitetris */
  { 0x7d124b8fu, 3u, 0u }, /* viu */
  { 0x8eee0f2fu, 5u, 0u }, /* vivid */
  { 0xa31f5354u, 3u, 0u }, /* vlc */
  { 0xb2b2616fu, 10u, 0u }, /* vobsub2srt */
  { 0xab51fd5cu, 12u, 0u }, /* vorbis-tools */
  { 0x795b3b5eu, 3u, 0u }, /* vtm */
  { 0x76935773u, 6u, 0u }, /* vttest */
  { 0x2933615eu, 22u, 0u }, /* vulkan-extension-layer */
  { 0x15e42131u, 14u, 0u }, /* vulkan-headers */
  { 0xc8cfb7fbu, 10u, 0u }, /* vulkan-icd */
  { 0x9ae692eau, 13u, 0u }, /* vulkan-loader */
  { 0x248853acu, 21u, 0u }, /* vulkan-loader-android */
  { 0x7c3a387au, 21u, 0u }, /* vulkan-loader-generic */
  { 0x43546644u, 12u, 0u }, /* vulkan-tools */
  { 0xe2e3c45du, 24u, 0u }, /* vulkan-utility-libraries */
  { 0xde9eb4a7u, 24u, 0u }, /* vulkan-validation-layers */
  { 0x1c17953bu, 11u, 0u }, /* vulkan-volk */
  { 0x2c17f436u, 3u, 0u }, /* w3m */
  { 0x8b2183d3u, 4u, 0u }, /* wabt */
  { 0x2360cf43u, 12u, 0u }, /* wakatime-cli */
  { 0xa0262210u, 4u, 0u }, /* walk */
  { 0x74a81fe7u, 7u, 0u }, /* wallust */
  { 0x42ed03b0u, 9u, 0u }, /* wasi-libc */
  { 0x6edbcbeeu, 17u, 0u }, /* wasm-component-ld */
  { 0x67c41184u, 8u, 0u }, /* wasmedge */
  { 0xcc4e883eu, 6u, 0u }, /* wasmer */
  { 0xa065460cu, 8u, 0u }, /* wasmtime */
  { 0xcf951f11u, 9u, 0u }, /* watchexec */
  { 0xcfbc3418u, 7u, 0u }, /* waypipe */
  { 0x334c9943u, 5u, 0u }, /* wcalc */
  { 0xbe29e715u, 5u, 0u }, /* wdiff */
  { 0x8bdb439cu, 18u, 0u }, /* webp-pixbuf-loader */
  { 0x78c9d73du, 8u, 0u }, /* websocat */
  { 0xb911077cu, 10u, 0u }, /* websocketd */
  { 0xa9321833u, 9u, 0u }, /* webtunnel */
  { 0x408b3bc2u, 7u, 0u }, /* weechat */
  { 0x793bf408u, 17u, 0u }, /* weechat-matrix-rs */
  { 0xbb061f98u, 6u, 0u }, /* weggli */
  { 0xd1cb0d3fu, 4u, 0u }, /* wego */
  { 0x01980a92u, 4u, 0u }, /* wget */
  { 0x2258b9e0u, 5u, 0u }, /* wget2 */
  { 0x86364f05u, 9u, 0u }, /* wgetpaste */
  { 0x08d2ece2u, 5u, 0u }, /* which */
  { 0xc4daf9a5u, 14u, 0u }, /* whitebox-tools */
  { 0xc363b0b3u, 5u, 0u }, /* whois */
  { 0x084726abu, 15u, 0u }, /* wireguard-tools */
  { 0x7b443dc8u, 9u, 0u }, /* wireproxy */
  { 0x1ff7ead5u, 3u, 0u }, /* wiz */
  { 0x07a54453u, 5u, 0u }, /* woff2 */
  { 0x2de8f655u, 3u, 0u }, /* wol */
  { 0x1847a50eu, 11u, 0u }, /* wordgrinder */
  { 0x71d1513fu, 4u, 0u }, /* wren */
  { 0x0eb4ea25u, 3u, 0u }, /* wrk */
  { 0x6ad6af7cu, 7u, 0u }, /* wtfutil */
  { 0x0ee004f9u, 4u, 0u }, /* wuzz */
  { 0x388da278u, 8u, 0u }, /* x11-repo */
  { 0x6824c251u, 9u, 0u }, /* xcb-proto */
  { 0xa0cf58f4u, 7u, 0u }, /* xdelta3 */
  { 0x4963683du, 2u, 0u }, /* xh */
  { 0x1b416df5u, 5u, 0u }, /* xmake */
  { 0xb9444b97u, 6u, 0u }, /* xmlsec */
  { 0xce139f6fu, 10u, 0u }, /* xmlstarlet */
  { 0x79694f2bu, 5u, 0u }, /* xmlto */
  { 0xd0ea7df7u, 5u, 0u }, /* xmppc */
  { 0x79216cecu, 16u, 0u }, /* xorg-util-macros */
  { 0xb4a23677u, 9u, 0u }, /* xorgproto */
  { 0xe79d5399u, 7u, 0u }, /* xorriso */
  { 0xbfd0bedbu, 6u, 0u }, /* xtrans */
  { 0x77cb70f1u, 8u, 0u }, /* xvidcore */
  { 0xa03e3719u, 6u, 0u }, /* xxhash */
  { 0x764eac94u, 4u, 0u }, /* yadm */
  { 0x7b3fa9e9u, 4u, 0u }, /* yajl */
  { 0x567b99d2u, 4u, 0u }, /* yara */
  { 0x597b9e8bu, 4u, 0u }, /* yarn */
  { 0x76798d9bu, 4u, 0u }, /* yasm */
  { 0x5e681eb2u, 4u, 0u }, /* yazi */
  { 0x80a3c0aau, 5u, 0u }, /* yosys */
  { 0xc2d011b4u, 9u, 0u }, /* youtubedr */
  { 0x4c5f9b5fu, 2u, 0u }, /* yq */
  { 0x6bf58bc4u, 10u, 0u }, /* yt-dlp-ejs */
  { 0x3ce3817au, 5u, 0u }, /* ytfzf */
  { 0x489a2f4cu, 10u, 0u }, /* ytui-music */
  { 0xd4572c4fu, 7u, 0u }, /* yuma123 */
  { 0xc2c80aa4u, 6u, 0u }, /* z-push */
  { 0x1e67a1bau, 2u, 0u }, /* z3 */
  { 0x70ddd5f2u, 4u, 0u }, /* zbar */
  { 0x203c2843u, 6u, 0u }, /* zellij */
  { 0xa10a2a86u, 7u, 0u }, /* zeronet */
  { 0x9a8258f1u, 3u, 0u }, /* zig */
  { 0xaf16fb1fu, 4u, 0u }, /* zile */
  { 0xab8273b4u, 3u, 0u }, /* zip */
  { 0x4cde8e71u, 6u, 0u }, /* zipios */
  { 0x4667e0b2u, 2u, 0u }, /* zk */
  { 0x7c5452c4u, 4u, 0u }, /* zlib */
  { 0x967a03e0u, 3u, 0u }, /* zls */
  { 0xb67eb36eu, 3u, 0u }, /* znc */
  { 0x0b95660du, 4u, 0u }, /* zola */
  { 0x6c9bc196u, 6u, 0u }, /* zoxide */
  { 0x5fdd0839u, 4u, 0u }, /* zpaq */
  { 0x396dfcd7u, 4u, 0u }, /* zrok */
  { 0xafc18e56u, 3u, 0u }, /* zsh */
  { 0xf6b3f9c8u, 15u, 0u }, /* zsh-completions */
  { 0xc59ce979u, 4u, 0u }, /* zssh */
  { 0xcfa9c52au, 4u, 0u }, /* zstd */
  { 0x9aa99ea2u, 5u, 0u }, /* zsync */
  { 0xdba11b8bu, 7u, 0u }, /* zziplib */
  { 0xc3449392u, 4u, 0u }, /* zzuf */

};

static const raf_termux_pkg_table_t g_table = {
  g_termux_pkgs,
  2055u,
  2166136261u,
  0x52414650u
};

const raf_termux_pkg_table_t *RmR_termux_packages(void){
  return &g_table;
}
