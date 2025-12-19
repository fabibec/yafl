# Use LuaLaTeX
$lualatex = 'lualatex -synctex=1 -interaction=errorstopmode %O %S';

$pdf_mode = 4;        # 4 = lualatex
$out_dir  = 'build';  # build directory

# Always use LuaLaTeX
$pdflatex = $lualatex;

