## Guidelines

## Mentality

* Performance is a key concern, not an afterthought
* If stuff needs to be broken to make it better don't be afraid, do it.

## Gpu API code

* If a function starts with `gpuapi_*` it is supposed to be a function that is available in at least one other API as well. This makes it easier to understand the work this function performs across different APIs, even if someone only has experience with one of the APIs. The parameters can still be different due to the inner workings of the API.

## Code Style

The following code style guidelines are the most important ones. These guidelines can be ignored if there is a very good reason.

* Function names must be lower snake case `my_function()`
* Macros must be upper snake case `MY_MACRO`
* Make functions `static` if they are only for internal use in the same file
* Use `struct` instead of `class`
* Functions should take structs as parameters instead of making them part of the struct ("avoid object orientated code")
* Avoid forward declaration where possible, simply order the functions and includes in the correct way
* C/C++ files should end with `.h` unless it is necessary to split structs and or definitions from the implementation (e.g. architecture dependent implementations). Then use `.h` and `.cpp`.
* Todos etc. can be written in comments.
