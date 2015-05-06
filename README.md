## crhash, a customizable hash cracker

crhash is a hackable hash cracker, meant to be customized for your special
cracking needs. Basically it's just a framework to enumerate strings given
a pattern and a charset and you can plug in your own hash computation and
checks.

It includes support for OpenCL, so if you want to use that you just have to
provide a kernel that computes a hash and checks whether it fulfills a certain
property. The framework will then run your kernel and find the strings that you
marked as positive.

Currently customization is done by tweaking the config.h header file. In the
future we might add the option to support compiled plugins in the form of
a shared library.

The included config searches for strings whose hexed MD5 hash is of the form
0eXXX..XX, where all the positions X have numeric values (0..9, not a..f).

### Usage

    Usage: crhash [FLAGS] pattern_string alphabet0 [alphabet1 [...]]

    pattern_string
      is a string with ? chars as placeholders

    alphabetX
      is an alphabet specification in one of two formats:
        =s     represents the explicit set of characters in s
        :lo:hi represents the set of characters with ASCII values
               in the inclusive range lo..hi

      You can provide different charsets for the different wildcard positions
      in the pattern string. The last character set you provide will be used
      for the remaining positions.

    OPTIONS
      -t integer  Use the given number of threads
      -s          No verbose output, just dump the result string
      -a          Find all matching strings
      -c          Use OpenCL. Currently only supports a subset of patterns,
                  specifically ones where the wildcards are all contiguous and
                  there is only one contiguous charset. I.e. <prefix>??...??<suffix>
      -h          Show this help

    EXAMPLES
      crhash -t 4 "My name is ???" :97:122
