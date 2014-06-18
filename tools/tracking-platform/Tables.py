# -*- coding: utf-8 -*-

if __name__ == '__main__':
    print '\tWARNING: You are trying to execute module which should be only imported.' 
    print '\tExiting...'
    raise SystemExit

from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import Column, Integer, String, ForeignKey
from sqlalchemy.orm import relationship, backref

_Base = declarative_base()


###############################################################################

def initializeTables(_engine):
    _Base.metadata.create_all(_engine) 

###############################################################################

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
    issue_hash          = Column(String)
    path                = relationship( "Path", backref="diagnostic" )

    line                = Column(Integer)
    column              = Column(Integer)    
    file_id             = Column(Integer, ForeignKey('files.id'))
    file                = relationship("files")


class Path(_Base):
    __tablename__ = 'paths'

    id                  = Column(Integer, primary_key=True)
    diagnostic_id       = Column(Integer, ForeignKey('diagnostics.id'))
    kind                = Column(String)
    message             = Column(String)
    extended_message    = Column(String)
    depth               = Column(Integer)

    line                = Column(Integer)
    column              = Column(Integer)    
    file_id             = Column(Integer, ForeignKey('files.id'))
    file                = relationship("files")


 
###############################################################################

