String toHex (uint8_t value) {
  if ( value < 16)
    return String("0") + String(value, HEX);
  else
    return String(value, HEX);
}

