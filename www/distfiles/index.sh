#!/bin/sh

printf 'Content-type: text/html; charset=utf-8\n\n'

cat << EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
	<head>
		<title>Herrie - distfiles</title>
		<style type="text/css">
			body, html {
				font-size: 11pt;
				font-family: Verdana, 'Bitstream Vera Sans', 'Lucida Grande', sans-serif;
			}
		</style>
	</head>
	<body>
		<h1>Tarballs - bzip2</h1>
		<ul>
EOF

for i in `ls -t herrie-*.bz2`
do
	stat -f "<li><a href=\"%N\">%N</a> (%z bytes)</li>" $i
done

cat << EOF
		</ul>

		<h1>Tarballs - gzip</h1>
		<ul>
EOF

for i in `ls -t herrie-*.gz`
do
	stat -f "<li><a href=\"%N\">%N</a> (%z bytes)</li>" $i
done

cat << EOF
		</ul>
	</body>
</html>
EOF
