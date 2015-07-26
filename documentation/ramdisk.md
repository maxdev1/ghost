## Ramdisk specification

The ramdisk can be created using the ramdisk building tool. The structure of this
ramdisk is as follows:

Entry types are:
    
     0 = FOLDER
     1 = FILE
    
Byte order is LITTLE ENDIAN. Filesystem node structure is:
    
      |-------------|-------------|
      | length      | name        |
      |-------------|-------------|
      | 1           | type        |
      | 4           | id          |
      | 4           | parentid    |
      | 4           | namelength  |
      | namelength  | name        |
      |             |             |
      |-------------|-------------|
      | files additionally have   |
      |-------------|-------------|
      | 4           | datalength  |
      | datalength  | data        |
      |-------------|-------------|
    
Files and folders are written into the file in no particular order.