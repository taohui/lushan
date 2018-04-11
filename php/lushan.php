<?php

/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */

function lushan_info($host, $port)
{
	$errno = 0;
	$errstr = "";
	$socket = fsockopen($host, $port, $errno, $errstr, 1.0);	
	if ($socket === FALSE) {
		return FALSE;
	}

	stream_set_timeout($socket, 1);

	$cmd = "info\r\n";
	$cmd_len = strlen($cmd);
	$ret = fwrite($socket, $cmd, $cmd_len);
	if ($ret === FALSE) {
		fclose($socket);
		return FALSE;
	}
	if ($ret != $cmd_len) {
		fclose($socket);
		return FALSE;
	}

	$line_no = 0;
	$table = array();
	$column_name = array();
	$column_name_num = 0;
	do {
		$line = fgets($socket);
		++$line_no;

		if (strcmp($line, "\r\n")) {
			if ($line_no == 1) {
				$line = trim($line);
				$column_name = preg_split("/\s+/", $line);
				$column_name_num = count($column_name);
			} else if ($line_no != 2) {
				$line = trim($line);
				$column_value = preg_split("/\s+/", $line);
				if ($column_name_num > 0 && 
					$column_name_num == count($column_value)) {

					$row = array();
					for($i = 0; $i < $column_name_num; $i++) {
						$row[$column_name[$i]] = $column_value[$i];
					}
					array_push($table, $row);
				} else if ($column_name_num > 0 &&

					$column_name_num == count($column_value) + 1) {
					$row = array();
					$j = 0;
					for($i = 0; $i < $column_name_num; $i++) {
						if ($i == 1) $row[$column_name[$i]] = "";
						else $row[$column_name[$i]] = $column_value[$j++];
					}
					array_push($table, $row);
				}
			}
		}
	} while(strcmp($line, "\r\n"));

	fclose($socket);

	return $table;
}

