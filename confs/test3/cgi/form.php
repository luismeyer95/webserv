#!/usr/bin/php-cgi

<?php
   if(isset($_POST['fname'], $_POST['lname'])) {
      echo "Welcome ". $_POST['fname']. "<br />";
      echo "Your last name is ". $_POST['lname']. ".";
      
      exit();
   }
?>

<html>
	<body>
		<form action="/form.php" method="POST">
			<label for="fname">First name:</label>
			<input type="text" id="fname" name="fname"><br><br>
			<label for="lname">Last name:</label>
			<input type="text" id="lname" name="lname"><br><br>
			<input type="submit" value="Submit">
		</form>
	</body>
</html>
