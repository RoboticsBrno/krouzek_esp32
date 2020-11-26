<?php echo "Ahoj!<br>"; ?>

<?php


$mysqli = new mysqli("mysql_server", "root", "tajneheslo", "mysql");
$res = $mysqli->query("SHOW DATABASES;");

while ($row = $res->fetch_assoc()) {
    print_r($row);
    echo "<br>";
}

?>
