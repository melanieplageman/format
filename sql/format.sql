CREATE EXTENSION hstore;
CREATE EXTENSION format;

SELECT format('Barry eats %(food)s', hstore('food', 'pork'));

