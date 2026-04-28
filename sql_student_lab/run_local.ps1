# Запуск лабораторной без Docker (Python 3 + sqlite3 из stdlib).
# Использование: из папки sql_student_lab выполнить: .\run_local.ps1

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$dataDir = Join-Path $root "data"
$db = Join-Path $dataDir "teach.db"
New-Item -ItemType Directory -Force -Path $dataDir | Out-Null
$env:DB_PATH = $db

if (-not (Test-Path $db)) {
    $initSql = Join-Path $root "sql\init.sql"
    $py = @"
import sqlite3
from pathlib import Path
p = Path(r'$db')
sql = Path(r'$initSql').read_text(encoding='utf-8')
con = sqlite3.connect(p)
con.executescript(sql)
con.commit()
con.close()
print('База создана:', p)
"@
    python -c $py
}

python (Join-Path $root "app\demo.py")
