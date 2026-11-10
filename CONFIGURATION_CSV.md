# Configuration CSV (S1TMS-17)

The application starts with built-in defaults and offers `load`, `edit`,
`start`, and `quit`. Loading parses and validates the entire file before
replacing the current settings. A failed load keeps the current settings. A
successful load displays the values and returns to the menu for further edits
or a run. Loading never saves automatically.

After a run, the application offers to save the effective configuration. The
next menu allows another run with current settings, editing, loading, saving
again, or quitting.

## Fixed CSV schema

Files contain exactly this header and one data row:

```csv
directions,north_density,south_density,east_density,west_density,green_ticks,yellow_ticks,queue_capacity,initial_vehicles,total_ticks
4,20,20,20,20,5,1,100,10,120
```

The four density fields always appear in North, South, East, West order. The
first `directions` fields must contain whole numbers in the density range; all
inactive direction fields must be empty. For example, two active directions
are written as:

```csv
2,20,15,,,5,1,100,10,120
```

Headers are case-sensitive and must match exactly. Unknown, reordered, or
duplicate headers, quoting, extra columns, extra data rows, missing values,
and overlong lines are rejected. Numeric ranges and cross-field constraints
match the interactive settings, including initial vehicles not exceeding queue
capacity. The file contains a single configuration row; discharge rate remains
the built-in default because it is not an editable setting.

On POSIX systems, saved files receive owner-only read/write permissions
(`0600`). On Windows, saved files inherit permissions from the destination
directory because portable C does not expose a standard ACL-setting API.
