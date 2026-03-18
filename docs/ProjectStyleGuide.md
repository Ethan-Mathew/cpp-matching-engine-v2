# Project Style Guide

I thought it would be prudent to summarize the style choices made while moving forward with this project. It's meant to serve as a tracker for the style choices, giving me something to refer back to as I progress and add new features. I try to include one of these in each of my personal projects, not just to solidify the standard used during project-creation, but also to monitor how I change my style as I become more experienced.

## Includes

I've been using the following hierarchy to order my library/header inclusions:

1. Inclusion of external libraries
2. Includes from include/
3. Includes from the local directory
4. Includes from the STL

## Blocks & Braces

For the sake of readability, I prefer to place both the opening and closing braces of function, class, struct, conditional, and enumeration bodies on separate lines. This has been my preference for some time, though I may eventually transition over to using inline opening braces due to their undeniable popularity.

## Classes & Structs

- Classes receive their public declarations first, then followed by private members.
- Member functions (apart from the special member functions).

## Enumerations

- Enumerated types are in all-caps.
- I'll prefer scoped enums.
