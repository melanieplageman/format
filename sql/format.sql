CREATE EXTENSION hstore;
CREATE EXTENSION format;

-- Format specifiers can occur anywhere in the format string

SELECT format('%(name)s Hello', hstore('name', 'World'));
SELECT format('<%(name)s> Hello', hstore('name', 'World'));

SELECT format('Hello %(name)s', hstore('name', 'World'));
SELECT format('Hello <%(name)s>', hstore('name', 'World'));

SELECT format('A %(name)s B', hstore('name', 'World'));

-- Format specifier with unmatched key

SELECT format('%(hi)s %(name)s', hstore('name', 'World'));

-- Format specifier with matched key but NULL value

SELECT format('Hello %(name)s', hstore('name', NULL));

-- Format specifier with punctuation in key name

SELECT format('Hello %(n@me)s', hstore('n@me', 'World'));

-- s, I, and L are type specifiers

SELECT format('%(name)s is %(type)s', hstore(ARRAY[
    'name', 'Melanie', 'type', 'cool']));

SELECT format('%(name)s is %(type)I', hstore(ARRAY[
    'name', 'Melanie', 'type', 'Cool']));

SELECT format('%(name)s is %(type)L', hstore(ARRAY[
    'name', 'Melanie', 'type', 'cool']));

SELECT format('%(name)I is %(type)I', hstore(ARRAY[
    'name', 'Melanie', 'type', 'Cool']));

SELECT format('%(name)L is %(type)L', hstore(ARRAY[
    'name', 'Melanie', 'type', 'cool']));

-- s, I, and L have special behavior on NULL values

SELECT format('%(null)s', hstore('null', NULL));
SELECT format('%(null)I', hstore('null', NULL));
-- TODO: this should return text 'NULL'
SELECT format('%(null)L', hstore('null', NULL));

-- Minimum width and alignment can be specified

SELECT format('%(name)-12L is %(type)L', hstore(ARRAY[
    'name', 'Melanie', 'type', 'cool']));

SELECT format('%(name)12L is %(type)L', hstore(ARRAY[
    'name', 'Melanie', 'type', 'cool']));

SELECT format('%(name)-12L is %(type)8L', hstore(ARRAY[
    'name', 'Melanie', 'type', 'cool']));
