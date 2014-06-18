#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
import Tables

def main():

	engine = create_engine('sqlite:///test.sqlite', echo=True)
	Tables.initializeTables( engine )
	
	SessionType = sessionmaker(bind=engine)
	session = SessionType()


if __name__ == '__main__':
	main()