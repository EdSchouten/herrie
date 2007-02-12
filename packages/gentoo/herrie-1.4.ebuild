DESCRIPTION="Herrie is a command line music player."
HOMEPAGE="http://g-rave.nl/projects/herrie/"
SRC_URI="http://g-rave.nl/projects/herrie/distfiles/${P}.tar.gz"

LICENSE="BSD"
SLOT="0"
KEYWORDS="~alpha ~amd64 ~ia64 ~mips ~ppc ~ppc64 ~sparc ~x86"
IUSE="ao mad scrobbler sndfile vorbis"

DEPEND="sys-libs/ncurses
		>=dev-libs/glib-2.0
		ao? ( media-libs/libao )
		mad? ( media-libs/libmad media-libs/libid3tag )
		scrobbler? ( net-misc/curl dev-libs/openssl )
		sndfile? ( media-libs/libsndfile )
		vorbis? ( media-libs/libvorbis )"

src_compile() {
	use ao && EXTRA_CONF="${EXTRA_CONF} ao"
	use mad || EXTRA_CONF="${EXTRA_CONF} no_mp3"
	use scrobbler || EXTRA_CONF="${EXTRA_CONF} no_scrobbler"
	use sndfile || EXTRA_CONF="${EXTRA_CONF} no_sndfile"
	use vorbis || EXTRA_CONF="${EXTRA_CONF} no_vorbis"
	
	./configure ${EXTRA_CONF} || die "configure failed"
	emake || die "make failed"
}

src_install() {
	dobin herrie
	doman herrie.1

	dodir /usr/share/locale/nl/LC_MESSAGES/
	insinto /usr/share/locale/nl/LC_MESSAGES/
	newins nl.mo herrie.mo
}
