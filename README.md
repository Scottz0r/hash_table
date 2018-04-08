# C Hash Table

Hash table implementation in C. This implementation allows for storage of any type of key and any type of data, with a configurable hashing function. Individual bytes of keys are used to hash data in the default implementation.
Functions that begin with "ht_strk" are convenience functions that assume keys are null terminated strings.

See tests.c for test coverage. Provided makefile creates an executable for running tests.
