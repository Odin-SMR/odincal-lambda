
=================
Intalling odincal
=================

Postgresql preparations
===========================

Make sure Postgresql is installed in your system.
  
  $> sudo apt-get install postgresql

Create a database a database called odin owned by your user.
  
  $> sudo -u postgres psql
  psql> create database odin owner <your_user_name>

Install the datamodel.

  $> psql -f create.sql

Short notes about the datamodel
===============================

FBA information connects to AC2 stw. Also the chopper wheel information is stored on AC2 stw.


