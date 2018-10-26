#ifndef ATOMSGAFFER_TYPEIDS_H
#define ATOMSGAFFER_TYPEIDS_H

namespace AtomsGaffer
{

enum class TypeId
{
	// make sure to claim a range with Cortex
	FirstTypeId = 120000,

	AtomsCrowdReaderTypeId = 120001,
	AtomsAgentReaderTypeId = 120002,
	AtomsCrowdGeneratorTypeId = 120003,

	LastTypeId = 120499,
};

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_TYPEIDS_H
