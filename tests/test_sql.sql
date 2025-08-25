--this is test sql file
select * from users;
select name, age from employees where age > 30;
update products set price = price * 1.1 where category = 'electronics';
insert into orders (user_id, product_id, quantity) values (1, 2, 3);
delete  from sessions where last_active < '2023-01-01';