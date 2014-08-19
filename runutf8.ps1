
$OutputEncoding = [Console]::OutputEncoding

$OutputEncoding

.\amain.exe  﨨狝 﨨狝 
.\amain.exe  﨨狝 﨨狝 | Set-Content -Encoding utf8 ajunk.txt

.\umain.exe  﨨狝 﨨狝 
.\umain.exe  﨨狝 﨨狝 | Set-Content -Encoding utf8 ujunk.txt

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$OutputEncodingU = [Console]::OutputEncoding

$OutputEncodingU

.\amain.exe  﨨狝 﨨狝 
.\amain.exe  﨨狝 﨨狝 | Set-Content -Encoding utf8 ajunk.txt

.\umain.exe  﨨狝 﨨狝 
.\umain.exe  﨨狝 﨨狝 | Set-Content -Encoding utf8 ujunk.txt

[Console]::OutputEncoding = $OutputEncoding
