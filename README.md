# format
This `format` function produces output formatted according to a format string (in a style similar to the C function `sprintf`).
This `format` is an extension of the built-in `format` function, as it is string interpolation with named format parameters.

For example, given an HStore with key 'location' and value 'World',

    SELECT format('Hello %(location)s!', hstore('location', 'World'));

    would produce the following:

    Hello World!

    Format specifiers are introduced by a % character followed by a paranthesized key name and take the form:

        %([key name])[flags][width]type
        where the component fields are:

        **key name** (required)  
        A valid HStore key

        **flags** (optional)  
        Additional options controlling how the format specifier's output is formatted. Currently the only supported flag is a minus sign (-) which will cause the format specifier's output to be left-justified. This has no effect unless the width field is also specified.

        **width** (optional)  
        Specifies the minimum number of characters to use to display the format specifier's output. The output is padded on the left or right (depending on the - flag) with spaces as needed to fill the width. A too-small width does not cause truncation of the output but is simply ignored. The width may be specified using a positive integer.

        **type** (required)  
        The type of format conversion to use to produce the format specifier's output. The following types are supported:

        * s formats the argument value as a simple string. A null value is treated as an empty string.

        * I treats the argument value as an SQL identifier, double-quoting it if necessary.

        * L quotes the argument value as an SQL literal.

        In addition to the format specifiers described above, the special sequence %% may be used to output a literal % character.
