//Chris Made this code.
//5/20/14

#include <rose.h>
#include <rose_p28.h>
#include <stp_schema.h>
#include <stix.h>
#include <string>
#include <map>
#include <iostream>
#include <cstdio>
#include <ARM.h>
#include <ctype.h>
#include <stix_asm.h>
#include <stix_tmpobj.h>
#include <stix_property.h>
#include <stix_split.h>

//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/xml_parser.hpp>

#include "track.h"
#include "ROSERange.h"
#pragma comment(lib,"stpcad_stix.lib")

using boost::property_tree::ptree;
using boost::property_tree::write_xml;
using boost::property_tree::xml_writer_settings;

unsigned uid = 0;
unsigned id_count = 1;

const std::string prefixes[] = { "exa", "peta", "tera", "giga", "mega", "kilo", "hecto", "deca", "deci", "centi", "milli", "micro", "nano", "pico", "femto", "atto" };
const std::string unit_names[] = { "metre",	"gram",	"second",	"ampere",	"kelvin",	"mole",	"candela",	"radian",	"steradian",	"hertz",	"newton",	"pascal",	"joule",	"watt",	"coulomb",	"volt",	"farad",	"ohm",	"siemens",	"weber",	"tesla",	"henry",	"degree_celsius",	"lumen",	"lux",	"becquerel",	"gray",	"sievert" };

void convertEntity(ptree* scope, RoseObject* ent, int currentUid){ //make it dumb so that it can be used on geomertry objects as well
	RoseObject* obj;
	auto atts = ent->attributes();
	if (!ent){ std::cout << "ent is null. WHERE IS YOUR GOD NOW! \n"; }
	for (unsigned i = 0, sz = atts->size(); i < sz; i++){
		RoseAttribute* att = atts->get(i);
		if (att->isEntity()){ //att points to another object //recurse!
			obj = ent->getObject(att);
			convertEntity(scope, obj, uid);
			//scope->add()
		}

		if (att->isAggregate()){//recurse!
			obj = ent->getObject(att);
			std::string derp;
			for (unsigned j = 0, sz = obj->size(); j < sz; j++){
				derp += std::to_string(obj->getDouble(j) ); 
				//std::cout << obj << "\n";
				derp += " ";
			}
			scope->add(att->name(), derp);
		}
		else if (att->isSelect()){//recurse!
			obj = rose_get_nested_object(ROSE_CAST(RoseUnion, ent->getObject(att)));
			std::cout << att->name() << "is select\n";
		}
		else if (att->isSimple()){//store this!
			if (att->isString()){
				if (strcmp("NONE", ent->getString(att))){
					scope->add(att->name(), ent->getString(att));
				}
			}
			else if(att->isInteger()){
				scope->add(att->name(), ent->getInteger(att) );
			}
			else if (att->isDouble()){
				scope->add(att->name(), ent->getDouble(att));
			}
			else if (att->isFloat()){
				scope->add(att->name(), ent->getFloat(att));
			}

		}
	}
}

void exchangeContext(ptree* scope, RoseDesign* des){
	unsigned i, sz;
	RoseCursor curse;
	RoseObject* obj;

	ptree& exchange = scope->add("ExchangeContext", "");
	exchange.add("<xmlattr>.uid", "ExchangeContext--" + std::to_string(uid));
	curse.domain(ROSE_DOMAIN(stp_language));
	curse.traverse(des);
	if (curse.size() > 0){ std::cout << curse.size() << "\n"; }
	else{ exchange.add("DefaultLanguage", "English"); }
	//units
	//deal with making ref to units later

	for (i = 0, sz = des->header_description()->description()->size(); i < sz; i++){
		exchange.add("Documentation", des->header_description()->description()->get(i));
	}
	curse.domain(ROSE_DOMAIN(stp_organization));
	curse.traverse(des);
	if (curse.size() > 0){
		obj = curse.next();
		exchange.add("IdentificationContext", obj->domain()->name() + std::string("--") + std::to_string(obj->entity_id()));
	}
}

void doUnits(Workpiece * wkpc, ptree* tree){
	auto gc = Geometric_context::find(wkpc->get_its_geometry()->context_of_items());
	auto l_u = gc->get_length_unit();
	if (l_u){
		//to get info out of units cast to type I want and just take it
		uidTracker* mgr = uidTracker::find(gc->get_length_unit());
		if (!mgr){
			mgr = uidTracker::make(gc->get_length_unit());
			uid++;
			std::string tmpUid = std::string("u--") + std::to_string(uid); 
			ptree& unit = tree->add("Unit", "");
			unit.add("<xmlattr>.uid", tmpUid);
			mgr->setUid(tmpUid);
			auto SI = ROSE_CAST(stp_si_unit, l_u);
			if (SI){
				unit.add("Kind.ClassString", "SI system");
				if (SI->name() >= 0){ unit.add("Name.ClassString", unit_names[SI->name()]); }
				if (SI->prefix() >= 0){ unit.add("Prefix.ClassString", prefixes[SI->prefix()]);	}
			}
		}
	}
	std::cout << gc->get_plane_angle_unit()->domain()->name() << "\n";
	auto pa_u = gc->get_plane_angle_unit();
	if (pa_u){
		uidTracker* mgr = uidTracker::find(gc->get_plane_angle_unit());
		if (!mgr){
			mgr = uidTracker::make(gc->get_plane_angle_unit());
			uid++;
			std::string tmpUid = std::string("u--") + std::to_string(uid);
			ptree& unit = tree->add("Unit", "");
			unit.add("<xmlattr>.uid", tmpUid);
			mgr->setUid(tmpUid);
			auto SI = ROSE_CAST(stp_si_unit, pa_u);
			auto conversion_unit = ROSE_CAST(stp_conversion_based_unit, pa_u);
			if (SI){
				unit.add("Kind.ClassString", "SI system");
				if (SI->name() >= 0){ unit.add("Name.ClassString", unit_names[SI->name()]); }
				if (SI->prefix() >= 0){ unit.add("Prefix.ClassString", prefixes[SI->prefix()]); }
			}
			else if (conversion_unit){
				unit.add("Kind.ClassString", "Conversion Based system");
				std::cout << conversion_unit->name() << "\n";
				unit.add("Name.ClassString", conversion_unit->name());

			}
		}
	}
	std::cout << gc->get_solid_angle_unit()->domain()->name() << "\n";
	auto sa_u = gc->get_solid_angle_unit();
	if (sa_u){
		uidTracker* mgr = uidTracker::find(gc->get_solid_angle_unit());
		if (!mgr){
			mgr = uidTracker::make(gc->get_solid_angle_unit());
			uid++;
			std::string tmpUid = std::string("u--") + std::to_string(uid);
			ptree& unit = tree->add("Unit", "");
			unit.add("<xmlattr>.uid", tmpUid);
			auto SI = ROSE_CAST(stp_si_unit, sa_u);

			if (SI){
				unit.add("Kind.ClassString", "SI system");
				if (SI->name() >= 0){ unit.add("Name.ClassString", unit_names[SI->name()]); }
				if (SI->prefix() >= 0){
					std::cout << SI->prefix() << "\n";
					unit.add("Prefix.ClassString", prefixes[SI->prefix()]);
				}
			}
		}
	}
}

void doShapeDependentProperty(Workpiece * wkpc, ptree* tree){ //waiting for joe to implement metafunctions for vole, surface area, centroid, etc..
	uid++;
	ptree& sdp = tree->add("ShapeDependentProperty", "");
	sdp.add("<xlmattr>.uid", "sdp--" + std::to_string(uid));
	sdp.add("<xlmattr>.xsi:type", "n1:GeneralShapeDependentProperty");
}

void doPartProperty(ptree* tree, RoseObject* ent){//filler code for demo
	uid++;
	stp_product_definition * pd = ROSE_CAST(stp_product_definition, ent);
	ptree& pv = tree->add("AssignedPropertyValue.PropertyValue", "");
	pv.add("<xmlattr>.uid", "pv--" + std::to_string(uid));
	pv.add("<xmlattr>.xsi:type", "n1:StringValue");
	pv.add("Definition.PropertyDefinitionString", "Part Origin"); // place of origin. is this for the design or manufacture of the part?
	pv.add("Name.CharacterString", pd->formation()->of_product()->name());
	pv.add("ValueComponent.CharacterString", "Step Tools Inc");
}

std::string handleGeometry(Workpiece * wkpc, ptree* tree){
	//returns the uid for the created geometricrepresentation  for use in PartView and storing geometry
	stp_shape_representation* srep = wkpc->get_its_geometry();
	uid++;
	int currentUid = uid;
	std::string uidForRef("gm--" + std::to_string(currentUid));
	ptree& geo = tree->add("n0:Uos.DataContainer.GeometricRepresentation", "");
	geo.add("<xmlattr>.uid", uidForRef);
	//ContextOfItems uidRef = coordinatespace(srep->context of items
	uid++;
	geo.add( srep->context_of_items()->className() + std::string(".<xmlattr>.uidRef"), "gcs--" + std::to_string(uid));
	ptree& dat = tree->add("n0:Uos.DataContainer.GeometricCoordinateSpace", "");
	dat.add("<xmlattr>.uid", "gcs--" + std::to_string(uid));
	auto gc = Geometric_context::find(srep->context_of_items());
	dat.add("DimensionCount", gc->get_dimensions());
	//handle units
	//handle getting uncertainty
	dat.add("Id.<xmlattr>.id", wkpc->get_its_id());
	
	ptree& items = geo.add("Items", "");
	for (unsigned j = 0, sz = srep->items()->size(); j < sz; j++){
		//children[j] is every RoseObject that is geometry
		RoseAttribute* att = srep->getAttribute("items");
		ptree& repItem = items.add(srep->items()->className(), "");
		uid++;
		repItem.add("<xmlattr>.uid", srep->items()->get(j) ->domain()->name() + std::string("--") + std::to_string(uid));
		repItem.add("<xmlattr>.xsi:type", std::string("n1:") + srep->items()->get(j)->domain()->name());
		convertEntity(&repItem , srep->items()->get(j), currentUid);
	}

	return uidForRef;
}

void makePart(Workpiece * wkpc, ptree* tree){
	// void makePart(stp_shape_definition_representation * sdr, ptree* tree){
	uid++;
	int currentUid = uid;
	RoseObject* obj;
	ListOfRoseObject children;
	stp_product_definition* pd;
	
	stp_shape_representation* srep = wkpc->get_its_geometry();
	pd = wkpc->getRoot();
	pd->findObjects(&children, INT_MAX, false);

	ptree& part = tree->add(std::string("n0:Uos.DataContainer.Part"), "");
	part.add("<xmlattr>.id", pd->domain()->name() + std::string("--") + std::to_string(currentUid));

	ptree& xmlObj = part.add("Id", "");
	xmlObj.add("<xmlattr>.id", wkpc->get_its_id() );

	xmlObj.add("Identifier", "");
	xmlObj.add("Identifier.<xmlattr>.uid", "pid--" + std::to_string(uid) + "--id" + std::to_string(id_count));

	xmlObj.add("Identifier.<xmlattr>.id", wkpc->get_its_id());
	xmlObj.add("Identifier.<xmlattr>.idContextRef", "create references");

	part.add("Name.CharacterString", wkpc->get_its_id());

	RoseCursor curse;
	curse.domain(ROSE_DOMAIN(stp_product_related_product_category));
	curse.traverse(pd->design());
	//RoseObject* obj;
	while (obj = curse.next()){
		stp_product_related_product_category* tmp = ROSE_CAST(stp_product_related_product_category, obj);
		if (tmp){
			//convertEntity(&part, tmp, uid);
			part.add("PartTypes.Class.<xmlattr>.uidRef", tmp->domain()->name() + std::string("--") + std::string(tmp->name()));
		}
	}
	part.add("Versions.PartVersion.Views.VersionOf", "p--" + std::to_string(currentUid));
	part.add("Versions.PartVersion.<xmlattr>.uid", "pv--" + std::to_string(currentUid) + "--id" + std::to_string(id_count));

	if (wkpc->get_revision_id() != NULL && strcmp(wkpc->get_revision_id(), "None") && strcmp(wkpc->get_revision_id(), "")){ //DO STRCMP or similarto check if none or empty
		part.add("Versions.Id.<xmlattr>.id", wkpc->get_revision_id());
	}
	else { part.add("Versions.Id.<xmlattr>.id", "/NULL"); } //replace null with a check for versioning that returns a string 

	ptree& pv = part.add("Versions.PartVersion.Views.PartView", "");

	pv.add("<xmlattr>.xsi:type", "n0:AssemblyDefinition");
	pv.add("<xmlattr>.uid", "pvv--" + std::to_string(currentUid) + "--id" + std::to_string(id_count));

	std::string geoRef = handleGeometry(wkpc, tree);	//ptree& geoRep = tree->add("n0:Uos.DataContainer.GeometricRepresentation", "");
	pv.put("DefiningGeometry.<xmlattr>.uidRef", geoRef);
	//do single occurence, 
	uidTracker* mgr = uidTracker::find(pd);
	if (mgr){
		if (mgr->getPV()){
			ptree& pi = pv.add("Occurrence", "");
			pi.add("<xmlattr>.xsi:type", "n0:SingleOccurrence");
			pi.add("<xmlattr>.uid", "pi--" + std::to_string(uid) + "--id" + std::to_string(id_count));
			pi.add("Id.<xmlattr>. id", pd->formation()->of_product()->name() + std::string(".") + std::to_string(mgr->getOccurence()));
			std::cout << "Occurrence: " << pd->formation()->of_product()->name() << "\n";
			pi.add("PropertyValueAssignment.<xmlattr>.uid", "pva--" + std::to_string(currentUid));
		}
	}

	StixMgrAsmProduct * pm = StixMgrAsmProduct::find(pd);
	for (unsigned i = 0; i < pm->child_nauos.size();i++){
		ptree& pi = pv.add("ViewOccurrenceRelationship", "");
		std::cout << stix_get_related_pdef(pm->child_nauos[i])->formation()->of_product()->name() << "\n";
		uidTracker* mgr = uidTracker::make(stix_get_related_pdef(pm->child_nauos[i])); //may need to label something different
		mgr->setPV(&pv);
		mgr->occurence++;
		std::cout << mgr->occurence << "\n";
		mgr->setUid("pi--" + std::to_string(uid) + "--id" + std::to_string(mgr->occurence));
		pi.add("<xmlattr>.uid", mgr->getUid());
		pi.add("<xmlattr>.xsi:type", std::string("n0:") + pm->child_nauos[i]->domain()->name());
		pi.add("Id.<xmlattr>.id", pm->child_nauos[i]->id() + std::string(".") + std::to_string(mgr->getOccurence()));
		pi.add("Description", pm->child_nauos[i]->description());
		pi.add("PropertyValueAssignment.<xmlattr>.uidRef", "pva--" + std::to_string(currentUid));
	}

	ptree& pva = pv.add("PropertyValueAssignment", "");
	pva.add("<xmlattr>.uid", "pva--" + std::to_string(currentUid));
	doPartProperty(&pva, pd);
	//stp_mass_measure_with_unit
	id_count++;
	return;
}

/*void do_nauo(Workpiece* wkpc, ptree* tree){
	stp_product_definition* pd = wkpc->getRoot();
	StixMgrAsmProduct * pm = StixMgrAsmProduct::find(pd);
	for (unsigned i = 0; i < pm->child_nauos.size(); i++){
		uidTracker* mgr = uidTracker::find(pd);
		if (mgr){
			ptree* pv = mgr->getPV();
			ptree& viewOcc = pv->add("ViewOccurrenceRelationship", "");
			viewOcc.add("<xmlattr>.uid", "");
		}
	}
}*/

void copyHeader(ptree* tree, RoseDesign* master){
	unsigned i, sz;
	//place holder. ask what needs to go in header on monday
	master->initialize_header();

	ptree& head = tree->add("n0:Uos.Header", "");
	head.put("<xmlattr>.xmlns", "");
	head.put("Name", master->name());
	for (i = 0, sz = master->header_name()->author()->size(); i < sz; i++){
		head.add("Author", master->header_name()->author()->get(i));
	}
	for (i = 0, sz = master->header_name()->author()->size(); i < sz; i++){
		head.add("Organization", master->header_name()->organization()->get(i));
	}
	head.put("PreprocessorVersion", master->header_name()->preprocessor_version());
	head.put("OriginatingSystem", master->header_name()->originating_system());
	head.put("Authorization", master->header_name()->authorisation());
	for (i = 0, sz = master->header_description()->description()->size(); i < sz; i++){
		head.put("Documentation", master->header_description()->description()->get(i));
	}


}

int main(int argc, char* argv[]){
	if (argc < 2){
		std::cout << "Usage: .\\STEPSplit.exe filetosplit.stp\n" << "\tCreates new file SplitOutput.stp as master step file with seperate files for each product" << std::endl;
		return EXIT_FAILURE;
	}
	ROSE.quiet(1);	//Suppress startup info.
	stplib_init();	// initialize merged cad library
	//    rose_p28_init();	// support xml read/write
	FILE *out;
	out = fopen("log.txt", "w");
	//ROSE.error_reporter()->error_file(out);
	RoseP21Writer::max_spec_version(PART21_ED3);	//We need to use Part21 Edition 3 otherwise references won't be handled properly.
	/* Create a RoseDesign to hold the output data*/
	std::string infilename(argv[1]);
	if (NULL == rose_dirname(infilename.c_str()))	//Check if there's already a path on the input file. If not, add '.\' AKA the local directory.
	{
		infilename = ".\\" + infilename;
	}
	if (!rose_file_readable(infilename.c_str()))	//Make sure file is readable before we open it.
	{
		std::cout << "Error reading input file." << std::endl;
		return EXIT_FAILURE;
	}
	RoseDesign * master = ROSE.useDesign(infilename.c_str());
	stix_tag_units(master);
	ARMpopulate(master);

	std::string name = "test.xml";
	std::string wrapperUrls[] = { "http://www.w3.org/2001/XMLSchema-instance", "http://standards.iso.org/iso/ts/10303/-3001/-ed-1/tech/xml-schema/bo_model", "http://standards.iso.org/iso/ts/10303/-3000/-ed-1/tech/xml-schema/common", "http://standards.iso.org/iso/ts/10303/-3001/-ed-1/tech/xml-schema/bo_model AP242_BusinessObjectModel.xsd" };
	std::string atts[] = { "xmlns:xsi", "xmlns:n0", "xmlns:cmn", "xsi:schemaLocation" };
	ptree tree;

	tree.add("n0:Uos.<xmlattr>." + atts[0], wrapperUrls[0]);
	for (int i = 1; i < 4; i++){
		tree.put("n0:Uos.<xmlattr>." + atts[i], wrapperUrls[i]);
	}

	//Add header
	copyHeader(&tree, master);
	ptree& dat = tree.add("n0:Uos.DataContainer", "");
	exchangeContext(&dat, master);
	dat.put("<xmlattr>.xmlns", "");
	dat.add("<xmlattr>.xsi:type", "n0:AP242DataContainer");

	stix_tag_asms(master);
	ARMCursor cur; //arm cursor
	ARMObject *a_obj;
	cur.domain(Workpiece::type());
	cur.traverse(master);
	ListOfRoseObject aimObjs;
	//unsigned i, sz;
	while (a_obj = cur.next()){
		std::cout << a_obj->getModuleName() << std::endl;
		doUnits(a_obj->castToWorkpiece(), &tree);
		makePart(a_obj->castToWorkpiece(), &tree);
	}
	cur.traverse(master);
	while (a_obj = cur.next()){
		//
	}

	//for (auto &i : ROSE_RANGE(stp_shape_definition_representation, master)){
		//makePart(&i, &tree);
	//}

	write_xml(std::string(master->fileDirectory() + name), tree, std::locale(), xml_writer_settings<char>(' ', 4));

	return 0;
};