#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import glob
import logging
import datetime
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
import sqlalchemy.orm.exc

import Tables

class DatabaseAccessProblem( BaseException ):
	pass

class NoInitializedDatabase( DatabaseAccessProblem ):
	pass

class CommandExecutor(object):
	def __init__( self, _containingDir ):
		self.__dbContainingDir = os.path.abspath( _containingDir )
		self.__dbPath = self.__dbContainingDir + '/' + self.getDatabaseName()
		self.__engine = None

	def getDatabaseName( self ):
		#return ".tracking-platform.db"
		return "tp_db.sqlite" 
	def getDBContainingDir( self ):
		return self.__dbContainingDir
	def getDBPath( self ):
		return self.__dbPath
	

	def checkDatabaseAvailability( self ):
		if not os.path.exists( self.getDBPath() ):
			raise NoInitializedDatabase()
		if not os.access( self.getDBPath(), os.W_OK ):
			raise DatabaseAccessProblem()
	def connect( self, _canBeNonexistent = False ):
		if not self.__engine:
			if not _canBeNonexistent:
				self.checkDatabaseAvailability()
			self.__engine = create_engine('sqlite:///' + self.getDBPath(), echo=False)
		Session = sessionmaker(bind=self.__engine)
		return Session()
	def getEngine( self ):
		if not self.__engine:
			raise RuntimeError()
		return self.__engine


	def execute( self ):
		raise RuntimeError()



class BaseLogCommandExecutor( CommandExecutor ):
	def __init__( self, **_args ):
		super( BaseLogCommandExecutor, self ).__init__( _args['containing_dir'] )
	
	def showDifference( self, _oldLaunch, _newLaunch ):
		newDiagnostics 	= set( _newLaunch.valid_diagnostics )
		oldDiagnostics 	= set( _oldLaunch.valid_diagnostics )
		fixed 			= oldDiagnostics.difference( newDiagnostics )
		notFixed 		= newDiagnostics.intersection( oldDiagnostics )
		new				= newDiagnostics.difference( oldDiagnostics )
		print (self.getIndent() + "Fixed diagnostics:           " + str( len( fixed )) 	) 
		print (self.getIndent() + "Not fixed diagnostics:       " + str( len( notFixed )) ) 
		print (self.getIndent() + "New diagnostics:             " + str( len( new )) 		) 

	def execute( self ):
		session = self.connect()
		return reversed( session.query( Tables.Launch ).all() )