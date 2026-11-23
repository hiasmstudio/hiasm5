db_file="elements.db"
sql_file="elements.sql"

if [ -f $db_file ]
then
    rm $db_file
fi
if [ -f $sql_file ]
then
	sqlite3 $db_file < $sql_file
else
    echo "Файл \"$sql_file\" не найден."
fi
