# FS Module

File operations for AKX.

## File Handle Operations

### `fs/open`
Opens a file, returns a File ID (FID).

**Args:** `:path "file.txt"` `:mode "r"`  
**Modes:** `r` (read), `w` (write), `a` (append), `r+`, `w+`, `a+`  
**Returns:** Integer FID  

### `fs/close`
Closes an open file.

**Args:** `:fid 1`  
**Returns:** `true`

### `fs/read`
Reads from an open file. Multiple modes:

**Byte mode:** `:fid 1 :bytes 100`  
**Line mode:** `:fid 1 :lines 5`  
**Range mode:** `:fid 1 :line-start 0 :line-end 10`  
**With offset:** `:fid 1 :offset 50 :bytes 20`  
**Whole file:** `:fid 1` (no other args)  
**Returns:** String content

### `fs/write`
Writes to an open file.

**Args:** `:fid 1 :content "data"`  
**Optional:** `:offset 10` (write at position), `:append 1` (append mode)  
**Returns:** Integer bytes written

### `fs/seek`
Moves file position.

**Args:** `:fid 1 :offset 100 :whence set`  
**Whence:** `set` (from start), `cur` (from current), `end` (from end)  
**Returns:** New position

### `fs/tell`
Gets current file position.

**Args:** `:fid 1`  
**Returns:** Integer position

## Simple File Operations

### `fs/read-file`
Reads entire file in one shot.

**Args:** `"path/to/file.txt"`  
**Returns:** String content

### `fs/write-file`
Writes entire file in one shot.

**Args:** `"path/to/file.txt" "content"`  
**Returns:** Integer bytes written

### `fs/?exists`
Checks if file/directory exists.

**Args:** `"path/to/file.txt"`  
**Returns:** `true` or `false`

### `fs/delete`
Deletes a file.

**Args:** `"path/to/file.txt"`  
**Returns:** `true` or `false`

## Path Operations

### `fs/path-join`
Joins path components.

**Args:** `"dir" "subdir" "file.txt"`  
**Returns:** `"dir/subdir/file.txt"`

### `fs/basename`
Gets filename from path.

**Args:** `"path/to/file.txt"`  
**Returns:** `"file.txt"`

### `fs/dirname`
Gets directory from path.

**Args:** `"path/to/file.txt"`  
**Returns:** `"path/to"`

## Notes

- FIDs are unique integers starting from 1
- File handles auto-increment, never reuse
- Close files when done (no auto-cleanup yet)
- Paths support `$VAR` environment variable expansion
- All keyword args use `:keyword value` syntax
- Escape sequences work: `\n`, `\t`, `\r`, `\\`, `\"`, `\0`

