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

-- Format specifier with matched key but null value

SELECT format('Hello %(name)s', hstore('name', NULL));

-- Format specifier with punctuation in key name

SELECT format('Hello %(na!me)s', hstore('na!me', NULL));

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


SELECT format('My name is %(name)12I and I like %(food)s', hstore(ARRAY[
    'name', 'Melanie', 'food', 'tacos']));
