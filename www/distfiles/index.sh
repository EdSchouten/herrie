#!/bin/sh

printf 'Content-type: text/html; charset=utf-8\n\n'

cat << EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
	<head>
		<title>Herrie - distfiles</title>
	</head>
	<body>
		<h1>Tarballs - bzip2</h1>
		<ul>
EOF

for i in `ls -t herrie-*.bz2`
do
	printf '\t\t\t<li><a href="%s">%s</a></li>\n' $i $i
done

cat << EOF
		</ul>

		<h1>Tarballs - gzip</h1>
		<ul>
EOF

for i in `ls -t herrie-*.gz`
do
	printf '\t\t\t<li><a href="%s">%s</a></li>\n' $i $i
done

cat << EOF
		</ul>
	</body>
</html>
EOF
