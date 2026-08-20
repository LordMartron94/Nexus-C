# nexus

## Bounded string writing

`NexusStringWriter` incrementally builds a null-terminated string in a
caller-owned buffer without allocating. Initialize it with
`nexus_strings_string_writer_initialize()`, then append literal or formatted
text with the corresponding append APIs.

Each append returns a `NexusStringFormatResult` for that append. The writer
retains its total written length and whether any append was truncated; obtain
those values with `nexus_strings_string_writer_length_get()` and
`nexus_strings_string_writer_truncated_get()`. Use it when one subsystem must
produce bounded diagnostic, serialization, or display text over multiple
operations.
