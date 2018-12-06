
import AtomsCore
import Atoms

class atomsRobotCached(Atoms.SimulationEvent):
	agentTypeFile = r'/home/alan/projects/atomscrowd/deploy/data/variations/cache/agentTypes/atomsRobot.agentType'
	eventName = 'atomsRobotCached'
	agentTypeName = 'atomsRobot'
	geoPath = ''
	skinPath = ''
	stateMachine = ''

	def __init__(self):
		Atoms.SimulationEvent.__init__(self)
		self.setName(self.eventName)

	def load(self):
		aType = Atoms.AgentType()
		aTypeArchive = AtomsCore.Archive()
		if aTypeArchive.readFromFile(self.agentTypeFile):
			aType.deserialise(aTypeArchive)
		else:
			return

		if self.geoPath:
			meshMap = AtomsCore.MapMetadata()
			typeArchive = AtomsCore.Archive()
			if typeArchive.readFromFile(self.geoPath):
				meshMap.deserialise(typeArchive)
				aType.metadata()["lowGeo"] = meshMap

		if self.skinPath:
			skinMap = AtomsCore.MapMetadata()
			skinArchive = AtomsCore.Archive()
			if skinArchive.readFromFile(self.skinPath):
				skinMap.deserialise(skinArchive)
				aType.metadata()["skinGeo"] = skinMap

		aType.metadata()["stateMachine"] = AtomsCore.StringMetadata(self.stateMachine)

		Atoms.AgentTypes.instance().addAgentType(self.agentTypeName, aType)
