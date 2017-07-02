create table student (
  sno char(8),
  sname char(16) unique,
  sage int,
  sgender char(1),
  primary key(sno)
);
insert into student values ('00001', 'Alice', 17, 'F');
insert into student values ('00002', 'Bob', 13, 'M');
insert into student values ('00003', 'Bob', 26, 'M');
insert into student values ('00004', 'Cindy', 25, 'F');
insert into student values ('00005', 'Dave', 16, 'M');
select * from student;
select * from student where sno = '00002';
select * from student where sage >= 15 and sage <= 25;
delete from student where sgender <> 'M';
select * from student;
drop table student;