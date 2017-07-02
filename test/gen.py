import random

print('''create table orders (
  orderkey int,
  custkey int unique,
  orderstatus char(1),
  totalprice float,
  clerk char(15),
  primary key(orderkey)
);
create index custkeyidx on orders(custkey);
''')

records = []
for i in range(0, 200000):
    records.append({
        'orderkey': i * 2,
        'custkey': i * 3,
        'orderstatus': 'AB'[random.randint(0, 1)],
        'totalprice': random.random() * 100,
        'clerk': ''.join(random.sample('ABCDEFGHIJKLMNOPQRSTUVWXYZ', 10))
    })
random.shuffle(records)

for record in records:
    print('insert into orders values(%s, %s, \'%s\', %s, \'%s\');' % (record['orderkey'], record['custkey'], record['orderstatus'], record['totalprice'], record['clerk']))
