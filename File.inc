<?php

/**
 * @file
 *
 * Helper functions for file processing.
 *
 * Notes:
 *  Beware of fseek() you can move it to the end of the file or beyond the end of the file, and feof() won't return the
 *  correct value as it depends on attempting to 'read' passed the end of the file to work.
 *
 *
 * End of line (EOL) sequences
 *  Windows end of line sequence:  \r\n
 *  Unix end of line sequence: \n
 *  Mac end of line sequence: \r
 */

/**
 * Moves the file pointer to the begining of the current line.
 *
 * If on the first line moves the pointer to 0.
 *
 * Any other line the pointer will be moved to the character following the previous line's EOL sequence as defined at
 * in this files header.
 *
 * The EOL is considered part of that line.
 *
 * @param resource $handle
 *   File handle.
 * @param int $read_size
 *   The amount to read in at time while searching for an EOL sequence. Tweak for performance.
 *
 * @return int
 *   The file position
 */
function file_line_start($handle, $read_size = 64) {
  /**
   * Ignore the cruft at the end of a file. This is important without it this function will assume the cruft
   * at the end of the file was a valid line.
   */
  if (file_eof($handle)) {
    file_end($handle);
  }
  // Ignore the eol that we are currently sitting on as its part of the current line.
  file_move_before_eol($handle);
  if (ftell($handle) == 0) { // Already at the start of a line by definition.
    return 0; // Ignore all other processing.
  }
  do { // Parse a section of the file looking for an EOL character from the previous line.
    $pos = ftell($handle);
    $length = min($pos, $read_size);
    $last_section = ($pos - $length) == 0;
    fseek($handle, -$length, SEEK_CUR);
    /**
     * Since we are looking for the character immediately following the EOL it not important if we get both
     * characters from Windows EOL
     */
    $s = fread($handle, $length); // Get section to parse.
    // Search for the last carriage return in this section
    $pattern = '/\r[^\r]*$/D'; // Last "\r"
    $matches = array();
    preg_match($pattern, $s, $matches, PREG_OFFSET_CAPTURE);
    $carriage_return_pos = isset($matches[0][1]) ? $matches[0][1] : NULL;
    // Search for the last new line in this section
    $pattern = '/\n[^\n]*$/D'; // Last "\n"
    $matches = array();
    preg_match($pattern, $s, $matches, PREG_OFFSET_CAPTURE);
    $new_line_pos = isset($matches[0][1]) ? $matches[0][1] : NULL;
    // Check if we found an EOL character
    $found_at = max($carriage_return_pos, $new_line_pos);
    // Move to the next section.
    fseek($handle, -$length, SEEK_CUR);
  } while (!$found_at && !$last_section);
  if ($found_at) { // Move pointer to character following the EOL
    fseek($handle, $found_at, SEEK_CUR);
    file_move_after_eol($handle);
  }
  return ftell($handle); // Current positon will be the character following the previous lines "\n" or the start of the file
}

/**
 * Moves the file pointer to the end of the current line, or the end of the file if and EOL sequence is not found.
 *
 * Beware of cases where the EOF is preceeded by a EOL sequence, in these cases the file pointer will be moved to
 * the EOL sequence.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 * @param int $read_size
 *   The amount to read in at time while searching for an EOL sequence. Tweak for performance.
 *
 * @return int
 *   The file position
 */
function file_line_end($handle, $read_size = 64) {
  if (file_eof($handle)) { // If at the EOF Ignore the cruft and return the EOF positon.
    file_end($handle);
    return ftell($handle);
  }
  if (file_eol($handle)) { // If at the EOL return the positon of the last character in the EOL sequence.
    return file_move_end_of_eol($handle);
  }
  do { // Parse a section of the file looking for an EOL character.
    $remaining = file_tell_eof($handle) - ftell($handle);
    $length = min($remaining, $read_size);
    $last_section = ($remaining - $length) == 0;
    $s = fread($handle, $length); // Get section to parse.
    // Search for the first carriage return in this section
    $pattern = '/^[^\r]*(\r)/D'; // First "\r"
    $matches = array();
    preg_match($pattern, $s, $matches, PREG_OFFSET_CAPTURE);
    $carriage_return_pos = isset($matches[1][1]) ? $matches[1][1] : NULL;
    // Search for the first new line in this section
    $pattern = '/^[^\n]*(\n)/D'; // First "\n"
    $matches = array();
    preg_match($pattern, $s, $matches, PREG_OFFSET_CAPTURE);
    $new_line_pos = isset($matches[1][1]) ? $matches[1][1] : NULL;
    // Check if we found an EOL character
    $found_at = min($carriage_return_pos, $new_line_pos);
  } while (!$found_at && !$last_section);
  if ($found_at) { // Move pointer to character following the EOL
    $offset = -($length - $found_at); // Move ahead one onto the EOL
    fseek($handle, $offset, SEEK_CUR);
    file_move_end_of_eol($handle);
  }
  return ftell($handle);
}

/**
 * Moves the file pointer to the begining of the previous line.
 *
 * This function can wrap the file, if called on the first line of the file the file pointer
 * will be placed at the last line of the file.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return int
 *   The file position
 */
function file_line_prev($handle) {
  $pos = file_line_start($handle); // Either at beginning of file or the character following an EOL.
  fungetc($handle); // Move back one character potentially onto the EOL, won't wrap the file.
  file_move_before_eol($handle);
  return file_line_start($handle);
}

/**
 * Moves the file pointer to the begining of the previous line.
 *
 * This function can wrap the file, if called on the last line of the file the file pointer
 * will be placed at the first line of the file.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return int
 *   The file position
 */
function file_line_next($handle) {
  $pos = file_line_end($handle);
  file_move_after_eol($handle);
  return ftell($handle);
}

/**
 * Moves the file pointer back the given number of $lines.
 *
 * The file pointer will be at the begining of the destination line.
 *
 * This function won't wrap the file, if the begining of the file is reached the function exits.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 * @param int $lines
 *   The max number of lines to move backward in the file.
 *
 * @return int
 *   The file position
 */
function file_move_back($handle, $lines) {
  $pos = file_line_start($handle);
  if ($pos != 0) {
    for ($i = 0; $i < $lines; $i++) {
      if (($pos = file_line_prev($handle)) == 0) {
        break;
      }
    }
  }
  return ftell($handle);
}

/**
 * Moves the file pointer forward the given number of $lines.
 *
 * The file pointer will be at the begining of the destination line.
 *
 * This function won't wrap the file, if the end of the file is reached the function exits.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 * @param int $lines
 *   The max number of lines to move forward in the file.
 *
 * @return int
 *   The file position
 */
function file_move_forward($handle, $lines) {
  $pos = file_line_start($handle);
  for ($i = 0; $i < $lines; $i++) {
    if (($pos = file_line_next($handle)) == 0) {
      file_line_prev($handle);
      break;
    }
  }
  return ftell($handle);
}

/**
 * Similar to fgetcsv, except it ignores enclosures, as fgetcvs breaks with open ended quotes.
 *
 * Also the number of expected fields can be given in which case the function will read multiple lines
 * until it has gotten the all the required fields. This helps deal with files that don't have properly
 * escaped newlines. The newlines will be preserved in the returned values.
 *
 * Please check too see if fgetcsv works before using this function as its scope is limited, it doesn't deal with
 * enclosures or escapes correctly.
 *
 * Note that it is possible for this function to return more fields than expected.
 *
 * @param resource $handle
 *   File handle.
 * @param int $length
 *   The max number of bytes to read from the current line.
 * @param string $delimiter
 *   A single character used to delimit the fields in the csv.
 * @param int $expected_fields
 *   The number of fields expected to be read for a single line.
 *
 * @return array
 *   An array containing the values in each field.
 */
function file_get_csv($handle, $length = 0, $delimiter = ',', $expected_fields = NULL) {
  $string = (isset($length) && $length > 0) ? fgets($handle, $length) : fgets($handle); // fget will issue a warning if given 0 or NULL for length.
  if ($string == FALSE) {
    return FALSE;
  }
  if (isset($expected_fields)) {
    $fields = explode($delimiter, $string);
    while (count($fields) < $expected_fields) {
      $string = (isset($length) && $length > 0) ? fgets($handle, $length) : fgets($handle); // fget will issue a warning if given 0 or NULL for length.
      if ($string == FALSE) {
        break;
      }
      $next_line_fields = explode($delimiter, $string);
      // Merge the last field with the start of this one.
      $fields[count($fields) - 1] .= array_shift($next_line_fields);
      $fields = array_merge($fields, $next_line_fields);
    }
  }
  else {
    $fields = explode($delimiter, $string); // No concern for the number of expected fields.
  }
  // Trim the last new line as it is meant to delimit the end of the row.
  $last_index = count($fields) - 1;
  $fields[$last_index] = trim($fields[$last_index]);
  return $fields;
}

/**
 * Similar to file_get_csv, except it moves backward though the file rather than forward.
 *
 * Also the number of expected fields can be given in which case the function will read multiple lines
 * until it has gotten the all the required fields. This helps deal with files that don't have properly
 * escaped newlines. The newlines will be preserved in the returned values.
 *
 * Please check too see if fgetcsv works before using this function as its scope is limitied, it doesn't deal with
 * enclosures or escapes correctly, it ignores them. By ignoring enclosures we can avoid fgetcsv() breaking on unclosed
 * quotes.
 *
 * Note that it is possible for this function to return more fields than expected.
 *
 * Becareful of trailing new lines at the end of csv files, and use of this function from arbitrary positions in a file.
 *
 * This function doesn't work in all the possible ways one might infer based on fgetcsv()
 *
 * @param resource $handle
 *   File handle.
 * @param int $length
 *   The max number of bytes to read from the current line.
 * @param string $delimiter
 *   A single character used to delimit the fields in the csv.
 * @param int $expected_fields
 *   The number of fields expected to be read for a single line.
 *
 * @return array
 *   An array containing the values in each field.
 */
function file_unget_csv($handle, $length = 0, $delimiter = ',', $expected_fields = NULL) {
  if (($string = fungets($handle, $length)) == FALSE) {
    return FALSE;
  }
  $fields = explode($delimiter, $string);
  if (isset($expected_fields) && $length == 0) { // Doesn't support length with $expected fields.
    while (count($fields) < $expected_fields) {
      if (($string = fungets($handle)) == FALSE) {
        break;
      }
      $next_line_fields = explode($delimiter, $string);
      // Merge the last field with the start of this one.
      $next_line_fields[count($next_line_fields) - 1] .= array_shift($fields);
      $fields = array_merge($next_line_fields, $fields);
    }
  }
  // Trim the last new line as it is meant to delimit the end of the row.
  $last_index = count($fields) - 1;
  $fields[$last_index] = trim($fields[$last_index]);
  return $fields;
}

/**
 * Moves the file pointer to the start of the file.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return boolean
 *   TRUE if successful, FALSE otherwise.
 */
function file_start($handle) {
  return fseek($handle, 0, SEEK_SET) == 0;
}

/**
 * Moves the file pointer to the end of the file.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return boolean
 *   TRUE if successful, FALSE otherwise.
 */
function file_end($handle) {
  return fseek($handle, 0, SEEK_END) == 0;
}

/**
 * If the file pointer is on a EOL sequence move it to before the sequence if possible.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return int
 *   The file position
 */
function file_move_before_eol($handle) {
  /**
   * If we are at the EOL of our current line ignore it. Notice the order since we are moving backward check for
   * "\n" before "\r" to catch all cases of EOL.
   */
  if (fpeekc($handle) == "\n") {
    fseek($handle, -1, SEEK_CUR);
  }
  if (fpeekc($handle) == "\r") {
    fseek($handle, -1, SEEK_CUR);
  }
  return ftell($handle);
}

/**
 * If the file pointer is on a EOL sequence move it to after the sequence if possible.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return int
 *   The file position.
 */
function file_move_after_eol($handle) {
  if (file_move_end_of_eol($handle) !== FALSE) {
    fseek($handle, 1, SEEK_CUR);
  }
  return ftell($handle);
}

/**
 * If the file pointer is on a EOL sequence move it to after the sequence if possible.
 *
 * This function assumes that the file pointer is currently on a EOL sequence when it is called. Use file_eol() to check
 * before calling this function. If the file pointer is not on a EOL sequence FALSE is returned.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return int
 *   The file position. Or FALSE if the file pointer wasn't on a EOL sequence.
 */
function file_move_end_of_eol($handle) {
  /**
   * Notice the order since we are moving forward check for
   * "\r\n" -> "\r" -> "\n" to catch all cases of EOL with multicharacters.
   */
  if (fpeekc($handle) == "\r") {
    fseek($handle, 1, SEEK_CUR);
    if (fpeekc($handle) != "\n") {
      fseek($handle, -1, SEEK_CUR); // Revert check for "\n"
    }
    return ftell($handle);
  }
  if (fpeekc($handle) == "\n") {
    return ftell($handle);
  }
  return FALSE;
}

/**
 * Checks if the file pointer is currently on an EOL sequence.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return boolean
 *   TRUE if successful, FALSE otherwise.
 */
function file_eol($handle) {
  return (fpeekc($handle) == "\r") || (fpeekc($handle) == "\n");
}

/**
 * feof() only works after a 'read' call like fgetc(), so functions like fseek() will invalidated it.
 *
 * This function will work the same as feof() except it does not require a 'read' call before.
 *
 * @param resource $handle
 *   File handle.
 */
function file_eof($handle) {
  $pos = ftell($handle);
  $c = fgetc($handle);
  fseek($handle, $pos, SEEK_SET);
  return $c === FALSE;
}

/**
 * Gets the position of the EOF character.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return int
 *   The position of the EOF character.
 */
function file_tell_eof($handle) {
  $pos = ftell($handle);
  file_end($handle);
  $eof_pos = ftell($handle);
  fseek($handle, $pos);
  return $eof_pos;
}

/**
 * Like fgetc() except after getting the current character the file pointer moves backward one instead of forward.
 *
 * Will not wrap around to the end the file.
 *
 * Returns FALSE when the character is EOF.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return string
 *   The single character read, or FALSE if EOF.
 */
function fungetc($handle) {
  $c = fpeekc($handle);
  fseek($handle, -1, SEEK_CUR); // Attempt to move to the previous character.
  return $c;
}

/**
 * Like fgets() except after getting the current character the file pointer moves backward one instead of forward.
 *
 * Will not wrap around to the end the file.
 *
 * Returns FALSE when the character is EOF.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 * @param int $length
 *   The max number of bytes to read from the current line, it must be a positive value greater than 0. This function
 *   will return $length - 1 characters.
 *
 * @return string
 *   The single character read, or FALSE if EOF or if there is an Error.
 */
function fungets($handle, $length = 0) {
  $pos = ftell($handle); // Store current position.
  $start = file_line_start($handle);
  $s = fpeeks($handle);
  $offset = $pos - $start; // We want to count the current character in the offset.
  $length = ($length > 0) ? min($length - 1, $offset) : $offset; // For consistency with fgets we treat length as starting from 1 rather than 0.
  $diff = $offset - $length;
  $s = drupal_substr($s, $offset - $length, $length); // Take out the chunk we are interested in.
  fseek($handle, $pos - $length - 1, SEEK_SET); // Move our pointer to character preceding the amount read out.
  return $s;
}

/**
 * Peeks at the current character.
 *
 * Doesn't move the file pointer.
 *
 * Returns FALSE when the character is EOF.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 *
 * @return string
 *   The single character read, or FALSE if EOF.
 */
function fpeekc($handle) {
  $pos = ftell($handle);
  $c = fgetc($handle);
  fseek($handle, $pos, SEEK_SET);
  return $c;
}

/**
 * Peeks at the current line.
 *
 * Doesn't move the file pointer.
 *
 * Reading ends when length - 1 bytes have been read, on a newline (which is included in the return value),
 * or on EOF (whichever comes first). If no length is specified, it will keep reading from the stream until
 * it reaches the end of the line.
 *
 * feof() will not work immeditely after this function is called. Use file_eof() instead.
 *
 * @param resource $handle
 *   File handle.
 * @param int $length
 *   The max number of bytes to read from the current line, it must be a positive value greater than 0.
 *
 * @return string
 *   The single character read, or FALSE if EOF.
 */
function fpeeks($handle, $length = 0) {
  $pos = ftell($handle);
  $s = $length > 0 ? fgets($handle, $length) : fgets($handle);
  fseek($handle, $pos, SEEK_SET);
  return $s;
}
