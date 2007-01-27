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
	use ao && MAKEOPTS="${MAKEOPTS} AUDIO_OUTPUT=ao"
	use mad || MAKEOPTS="${MAKEOPTS} NO_MP3=YES"
	use scrobbler || MAKEOPTS="${MAKEOPTS} NO_SCROBBLER=YES"
	use sndfile || MAKEOPTS="${MAKEOPTS} NO_SNDFILE=YES"
	use vorbis || MAKEOPTS="${MAKEOPTS} NO_VORBIS=YES"
	emake ${MAKEOPTS}
}

src_install() {
	dobin herrie
	doman herrie.1

	dodir /usr/share/locale/nl/LC_MESSAGES/
	insinto /usr/share/locale/nl/LC_MESSAGES/
	newins nl.mo herrie.mo
}
