# -*- coding: utf-8 -*-

if __name__ == '__main__':
    print '\tWARNING: You are trying to execute module which should be only imported.' 
    print '\tExiting...'
    raise SystemExit

from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import Table, Column, Integer, String, DateTime, ForeignKey
from sqlalchemy.orm import relationship, backref

_Base = declarative_base()


###############################################################################

def initializeTables(_engine):
    _Base.metadata.create_all(_engine) 

###############################################################################

launches_diagnostics = Table('launches_diagnostics', _Base.metadata,
    Column('launch_id', Integer, ForeignKey('launches.id')),
    Column('diagnostic_id', Integer, ForeignKey('diagnostics.id'))
)

class Files(_Base):
    __tablename__ = 'files'

    id      = Column(Integer, primary_key=True)
    name    = Column(String)
    
class Diagnostic(_Base):
    __tablename__ = 'diagnostics'

    id                  = Column(Integer, primary_key=True)
    description         = Column(String)
    category            = Column(String)
    type                = Column(String)
    issue_context_kind  = Column(String)
    issue_context       = Column(String) 
    issue_hash          = Column(String, unique=True)
    path                = relationship( "Path", backref="diagnostic" )

    line                = Column(Integer)
    column              = Column(Integer)    
    file_id             = Column(Integer, ForeignKey('files.id'))
    file                = relationship("files")
    launches            = relationship("launches", secondary=launches_diagnostics)

class Path(_Base):
    __tablename__ = 'paths'

    id                  = Column(Integer, primary_key=True)
    diagnostic_id       = Column(Integer, ForeignKey('diagnostics.id'))
    kind                = Column(String)
    message             = Column(String)
    extended_message    = Column(String)
    depth               = Column(Integer)
    piece_id            = Column(Integer)

    line                = Column(Integer)
    col                 = Column(Integer)    
    file_id             = Column(Integer, ForeignKey('files.id'))
    file                = relationship("files")

class Launch(_Base):
    __tablename__ = 'launches'
    id                  = Column(Integer, primary_key=True)
    ## Some other information...
    time                = Column(DateTime)
    diagnostics         = relationship("diagnostics", secondary=launches_diagnostics)

 
###############################################################################

