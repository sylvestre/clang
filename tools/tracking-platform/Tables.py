# -*- coding: utf-8 -*-

if __name__ == '__main__':
    print '\tWARNING: You are trying to execute module which should be only imported.' 
    print '\tExiting...'
    raise SystemExit

from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import Table, Column, Integer, String, DateTime, ForeignKey, UniqueConstraint
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

launches_pathpieces = Table('launches_pathpieces', _Base.metadata,
    Column('launch_id', Integer, ForeignKey('launches.id')),
    Column('pathpiece_id', Integer, ForeignKey('path_pieces.id'))
)

class File(_Base):
    __tablename__ = 'files'

    id      = Column(Integer, primary_key=True)
    name    = Column(String)
    def __repr__(self):
        return "%s" % (self.name)


class Diagnostic(_Base):
    __tablename__ = 'diagnostics'

    id                  = Column(Integer, primary_key=True)
    description         = Column(String)
    category            = Column(String)
    type                = Column(String)
    issue_context_kind  = Column(String)
    issue_context       = Column(String) 
    issue_hash          = Column(String, unique=True)
    
    class IllegalLaunch( BaseException ):
        pass

    all_path_pieces     = relationship("PathPiece")

    def getPath( self, _launch ):
        if _launch not in self.launches:
            raise IllegalLaunch()
        proper_piece_ids = object_session(self).select( launches_pathpieces.pathpiece_id ).where( launches_pathpieces.launches == _launch.id ).all()
        return object_session(self).select( PathPiece ).filter( (PathPiece.diagnostic_id == self.id) and (PathPiece.id in proper_piece_ids) ).all()

    line                = Column(Integer)
    col                 = Column(Integer)    
    file_id             = Column(Integer, ForeignKey('files.id'))
    file                = relationship("File")
    launches            = relationship("Launch", secondary=launches_diagnostics, backref='diagnostics')

    def __repr__(self):
        return    ("Description: %s\n"
                  "Category:    %s\n"
                  "Type:        %s\n"
                  "Unique ID:   %s\n"
                  "Location:    %s(%d:%d)\n")  % (self.description, self.category, self.type, self.issue_hash, self.file, self.line, self.col)

class PathPiece(_Base):
    __tablename__ = 'path_pieces'

    id                  = Column(Integer, primary_key=True)
    diagnostic_id       = Column(Integer, ForeignKey('diagnostics.id'))
    message             = Column(String)
    extended_message    = Column(String)
    depth               = Column(Integer)
    piece_id            = Column(Integer)
    # TODO: add optional isKeyEvent
    # TODO: add optional ranges
    line                = Column(Integer)
    col                 = Column(Integer)    
    file_id             = Column(Integer, ForeignKey('files.id'))
    file                = relationship("File")
    launches            = relationship("Launch", secondary=launches_pathpieces)

    __table_args__ = ( UniqueConstraint('diagnostic_id', 'message','extended_message','depth','piece_id','line','col','file_id'), )


class Launch(_Base):
    __tablename__ = 'launches'
    id                  = Column(Integer, primary_key=True)
    ## Some other information...
    time                = Column(DateTime)


    name                = Column(String) # Name of the directory with results
    def __repr__(self):
        return "Scan-build's launch #%d on %s \nloaded from %s" % (self.id, self.time.strftime("%b %d %Y %H:%M:%S"), self.name)

 
###############################################################################

