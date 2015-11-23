/*
 * libpri: An implementation of Primary Rate ISDN
 *
 * Copyright (C) 2009 Digium, Inc.
 *
 * Richard Mudgett <rmudgett@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2 as published by the
 * Free Software Foundation. See the LICENSE file included with
 * this program for more details.
 *
 * In addition, when this program is distributed with Asterisk in
 * any form that would qualify as a 'combined work' or as a
 * 'derivative work' (but not mere aggregation), you can redistribute
 * and/or modify the combination under the terms of the license
 * provided with that copy of Asterisk, instead of the license
 * terms granted here.
 */

/*!
 * \file
 * \brief ROSE encode/decode test program
 *
 * \author Richard Mudgett <rmudgett@digium.com>
 */


#include "compat.h"
#include "libpri.h"
#include "pri_internal.h"
#include "rose.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------- */


static const struct fac_extension_header fac_headers[] = {
/* *INDENT-OFF* */
	{
		.nfe_present = 0,
	},
	{
		.nfe_present = 1,
		.nfe.source_entity = 1,
		.nfe.destination_entity = 1,
	},
	{
		.nfe_present = 1,
		.nfe.source_entity = 1,
		.nfe.source_number.plan = 4,
		.nfe.source_number.length = 4,
		.nfe.source_number.str = "9834",
		.nfe.destination_entity = 1,
		.nfe.destination_number.plan = 4,
		.nfe.destination_number.length = 4,
		.nfe.destination_number.str = "9834",
	},
	{
		.nfe_present = 1,
		.nfe.source_entity = 1,
		.nfe.destination_entity = 1,
		.npp_present = 1,
		.npp = 19,
		.interpretation_present = 1,
		.interpretation = 2,
	},
/* *INDENT-ON* */
};


static const struct rose_message rose_etsi_msgs[] = {
/* *INDENT-OFF* */
	/* Reject messages */
	{
		.type = ROSE_COMP_TYPE_REJECT,
		.component.reject.code = ROSE_REJECT_Gen_BadlyStructuredComponent,
	},
	{
		.type = ROSE_COMP_TYPE_REJECT,
		.component.reject.invoke_id_present = 1,
		.component.reject.invoke_id = 10,
		.component.reject.code = ROSE_REJECT_Inv_InitiatorReleasing,
	},
	{
		.type = ROSE_COMP_TYPE_REJECT,
		.component.reject.invoke_id_present = 1,
		.component.reject.invoke_id = 11,
		.component.reject.code = ROSE_REJECT_Res_MistypedResult,
	},
	{
		.type = ROSE_COMP_TYPE_REJECT,
		.component.reject.invoke_id_present = 1,
		.component.reject.invoke_id = 12,
		.component.reject.code = ROSE_REJECT_Err_ErrorResponseUnexpected,
	},

	/* Anonymous result or result without any arguments. */
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_None,
		.component.result.invoke_id = 9,
	},

	/* Advice Of Charge (AOC) */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_ChargingRequest,
		.component.invoke.invoke_id = 98,
		.component.invoke.args.etsi.ChargingRequest.charging_case = 2,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 99,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.special_charging_code = 3,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 100,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.currency = "Dollars",
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.amount.currency = 7,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.amount.multiplier = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.charging_type = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.time.length = 8,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.time.scale = 4,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 101,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.currency = "Dollars",
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.amount.currency = 7,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.amount.multiplier = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.charging_type = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.time.length = 8,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.time.scale = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.granularity_present = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.granularity.length = 20,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.duration.granularity.scale = 3,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 102,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 2,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.flat_rate.currency = "Euros",
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.flat_rate.amount.currency = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.flat_rate.amount.multiplier = 1,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 103,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 3,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.volume_rate.currency = "Yen",
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.volume_rate.amount.currency = 300,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.volume_rate.amount.multiplier = 5,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.volume_rate.unit = 2,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 104,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 2,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 2,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.flat_rate.currency = "Euros",
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.flat_rate.amount.currency = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].u.flat_rate.amount.multiplier = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[1].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[1].currency_type = 3,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[1].u.volume_rate.currency = "Yen",
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[1].u.volume_rate.amount.currency = 300,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[1].u.volume_rate.amount.multiplier = 5,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[1].u.volume_rate.unit = 2,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 105,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 4,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_ETSI_ChargingRequest,
		.component.result.invoke_id = 106,
		.component.result.args.etsi.ChargingRequest.type = 0,
		.component.result.args.etsi.ChargingRequest.u.currency_info.num_records = 1,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].charged_item = 4,
		.component.result.args.etsi.ChargingRequest.u.currency_info.list[0].currency_type = 5,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCSCurrency,
		.component.invoke.invoke_id = 107,
		.component.invoke.args.etsi.AOCSCurrency.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCSCurrency,
		.component.invoke.invoke_id = 108,
		.component.invoke.args.etsi.AOCSCurrency.type = 1,
		.component.invoke.args.etsi.AOCSCurrency.currency_info.num_records = 1,
		.component.invoke.args.etsi.AOCSCurrency.currency_info.list[0].charged_item = 3,
		.component.invoke.args.etsi.AOCSCurrency.currency_info.list[0].currency_type = 4,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCSSpecialArr,
		.component.invoke.invoke_id = 109,
		.component.invoke.args.etsi.AOCSSpecialArr.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCSSpecialArr,
		.component.invoke.invoke_id = 110,
		.component.invoke.args.etsi.AOCSSpecialArr.type = 1,
		.component.invoke.args.etsi.AOCSSpecialArr.special_arrangement = 9,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDCurrency,
		.component.invoke.invoke_id = 111,
		.component.invoke.args.etsi.AOCDCurrency.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDCurrency,
		.component.invoke.invoke_id = 112,
		.component.invoke.args.etsi.AOCDCurrency.type = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDCurrency,
		.component.invoke.invoke_id = 113,
		.component.invoke.args.etsi.AOCDCurrency.type = 2,
		.component.invoke.args.etsi.AOCDCurrency.specific.recorded.currency = "Francs",
		.component.invoke.args.etsi.AOCDCurrency.specific.recorded.amount.currency = 674,
		.component.invoke.args.etsi.AOCDCurrency.specific.recorded.amount.multiplier = 3,
		.component.invoke.args.etsi.AOCDCurrency.specific.type_of_charging_info = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDCurrency,
		.component.invoke.invoke_id = 114,
		.component.invoke.args.etsi.AOCDCurrency.type = 2,
		.component.invoke.args.etsi.AOCDCurrency.specific.recorded.currency = "Francs",
		.component.invoke.args.etsi.AOCDCurrency.specific.recorded.amount.currency = 674,
		.component.invoke.args.etsi.AOCDCurrency.specific.recorded.amount.multiplier = 3,
		.component.invoke.args.etsi.AOCDCurrency.specific.type_of_charging_info = 1,
		.component.invoke.args.etsi.AOCDCurrency.specific.billing_id_present = 1,
		.component.invoke.args.etsi.AOCDCurrency.specific.billing_id = 2,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDChargingUnit,
		.component.invoke.invoke_id = 115,
		.component.invoke.args.etsi.AOCDChargingUnit.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDChargingUnit,
		.component.invoke.invoke_id = 116,
		.component.invoke.args.etsi.AOCDChargingUnit.type = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDChargingUnit,
		.component.invoke.invoke_id = 117,
		.component.invoke.args.etsi.AOCDChargingUnit.type = 2,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].not_available = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.type_of_charging_info = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDChargingUnit,
		.component.invoke.invoke_id = 118,
		.component.invoke.args.etsi.AOCDChargingUnit.type = 2,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].not_available = 0,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].number_of_units = 8523,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.type_of_charging_info = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.billing_id_present = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.billing_id = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDChargingUnit,
		.component.invoke.invoke_id = 119,
		.component.invoke.args.etsi.AOCDChargingUnit.type = 2,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].not_available = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].type_of_unit_present = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].type_of_unit = 13,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.type_of_charging_info = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDChargingUnit,
		.component.invoke.invoke_id = 120,
		.component.invoke.args.etsi.AOCDChargingUnit.type = 2,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].not_available = 0,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].number_of_units = 8523,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].type_of_unit_present = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].type_of_unit = 13,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.type_of_charging_info = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCDChargingUnit,
		.component.invoke.invoke_id = 121,
		.component.invoke.args.etsi.AOCDChargingUnit.type = 2,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.num_records = 2,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[0].not_available = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[1].not_available = 0,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[1].number_of_units = 8523,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[1].type_of_unit_present = 1,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.recorded.list[1].type_of_unit = 13,
		.component.invoke.args.etsi.AOCDChargingUnit.specific.type_of_charging_info = 1,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 122,
		.component.invoke.args.etsi.AOCECurrency.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 123,
		.component.invoke.args.etsi.AOCECurrency.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.free_of_charge = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 124,
		.component.invoke.args.etsi.AOCECurrency.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.free_of_charge = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association_present = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.type = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.id = -37,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 125,
		.component.invoke.args.etsi.AOCECurrency.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.free_of_charge = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association_present = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.number.plan = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.number.length = 7,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.number.str = "5551212",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 126,
		.component.invoke.args.etsi.AOCECurrency.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.free_of_charge = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.currency = "Francs",
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.currency = 674,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.multiplier = 3,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 127,
		.component.invoke.args.etsi.AOCECurrency.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.free_of_charge = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.currency = "Francs",
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.currency = 674,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.multiplier = 3,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association_present = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.type = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.id = -37,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 128,
		.component.invoke.args.etsi.AOCECurrency.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.free_of_charge = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.currency = "Francs",
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.currency = 674,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.multiplier = 3,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.billing_id_present = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.billing_id = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCECurrency,
		.component.invoke.invoke_id = 129,
		.component.invoke.args.etsi.AOCECurrency.type = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.free_of_charge = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.currency = "Francs",
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.currency = 674,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.recorded.amount.multiplier = 3,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.billing_id_present = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.specific.billing_id = 2,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association_present = 1,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.type = 0,
		.component.invoke.args.etsi.AOCECurrency.currency_info.charging_association.id = -37,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCEChargingUnit,
		.component.invoke.invoke_id = 130,
		.component.invoke.args.etsi.AOCEChargingUnit.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCEChargingUnit,
		.component.invoke.invoke_id = 131,
		.component.invoke.args.etsi.AOCEChargingUnit.type = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.free_of_charge = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCEChargingUnit,
		.component.invoke.invoke_id = 132,
		.component.invoke.args.etsi.AOCEChargingUnit.type = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.free_of_charge = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association_present = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association.type = 0,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association.id = -37,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCEChargingUnit,
		.component.invoke.invoke_id = 133,
		.component.invoke.args.etsi.AOCEChargingUnit.type = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.free_of_charge = 0,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.list[0].not_available = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCEChargingUnit,
		.component.invoke.invoke_id = 134,
		.component.invoke.args.etsi.AOCEChargingUnit.type = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.free_of_charge = 0,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.list[0].not_available = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association_present = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association.type = 0,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association.id = -37,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCEChargingUnit,
		.component.invoke.invoke_id = 135,
		.component.invoke.args.etsi.AOCEChargingUnit.type = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.free_of_charge = 0,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.list[0].not_available = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.billing_id_present = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.billing_id = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_ETSI_AOCEChargingUnit,
		.component.invoke.invoke_id = 136,
		.component.invoke.args.etsi.AOCEChargingUnit.type = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.free_of_charge = 0,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.num_records = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.recorded.list[0].not_available = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.billing_id_present = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.specific.billing_id = 2,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association_present = 1,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association.type = 0,
		.component.invoke.args.etsi.AOCEChargingUnit.charging_unit.charging_association.id = -37,
	},
/* *INDENT-ON* */
};

static unsigned char rose_etsi_indefinite_len[] = {
/* *INDENT-OFF* */
/*
 *	Context Specific/C [1 0x01] <A1> Len:24 <80>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<44>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<07>
 *		Sequence/C(48 0x30) <30> Len:16 <80>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<01>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<05>
 *			Sequence/C(48 0x30) <30> Len:6 <80>
 *				Context Specific [4 0x04] <84> Len:4 <80>
 *					<31 38 30 33>
 *				0x00, 0x00,
 *			0x00, 0x00,
 *			NULL(5 0x05) <05> Len:0 <00>
 *		0x00, 0x00,
 *	0x00, 0x00
 */
	0x91,
	0xA1, 0x80,
		0x02, 0x01,
			0x44,
		0x02, 0x01,
			0x07,
		0x30, 0x80,
			0x0A, 0x01,
				0x01,
			0x0A, 0x01,
				0x05,
			0x30, 0x80,
				0x84, 0x80,
					0x31, 0x38, 0x30, 0x33,
				0x00, 0x00,
			0x00, 0x00,
			0x05, 0x00,
		0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00
/* *INDENT-ON* */
};

static unsigned char rose_etsi_unused_indefinite_len[] = {
/* *INDENT-OFF* */
/*
 *	Context Specific/C [1 0x01] <A1> Len:24 <80>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<44>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<06> -- EctExecute
 *		Sequence/C(48 0x30) <30> Len:16 <80>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<01>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<05>
 *			Sequence/C(48 0x30) <30> Len:6 <80>
 *				Context Specific [4 0x04] <84> Len:4 <80>
 *					<31 38 30 33>
 *				0x00, 0x00,
 *			0x00, 0x00,
 *			NULL(5 0x05) <05> Len:0 <00>
 *		0x00, 0x00,
 *	0x00, 0x00
 */
	0x91,
	0xA1, 0x80,
		0x02, 0x01,
			0x44,
		0x02, 0x01,
			0x06,
		0x30, 0x80,
			0x0A, 0x01,
				0x01,
			0x0A, 0x01,
				0x05,
			0x30, 0x80,
				0x84, 0x80,
					0x31, 0x38, 0x30, 0x33,
				0x00, 0x00,
			0x00, 0x00,
			0x05, 0x00,
		0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00
/* *INDENT-ON* */
};

static unsigned char rose_etsi_unused[] = {
/* *INDENT-OFF* */
/*
 *	Context Specific/C [1 0x01] <A1> Len:24 <18>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<44>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<06> -- EctExecute
 *		Sequence/C(48 0x30) <30> Len:16 <10>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<01>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<05>
 *			Sequence/C(48 0x30) <30> Len:6 <06>
 *				Context Specific [4 0x04] <84> Len:4 <04>
 *					<31 38 30 33>
 *			NULL(5 0x05) <05> Len:0 <00>
 */
	0x91,
	0xA1, 0x18,
		0x02, 0x01,
			0x44,
		0x02, 0x01,
			0x06,
		0x30, 0x10,
			0x0A, 0x01,
				0x01,
			0x0A, 0x01,
				0x05,
			0x30, 0x06,
				0x84, 0x04,
					0x31, 0x38, 0x30, 0x33,
			0x05, 0x00,
	0x00, 0x00
/* *INDENT-ON* */
};

static unsigned char rose_etsi_extra[] = {
/* *INDENT-OFF* */
/*
 *	Context Specific/C [1 0x01] <A1> Len:24 <18>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<44>
 *		Integer(2 0x02) <02> Len:1 <01>
 *			<07>
 *		Sequence/C(48 0x30) <30> Len:16 <10>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<01>
 *			Enumerated(10 0x0A) <0A> Len:1 <01>
 *				<05>
 *			Sequence/C(48 0x30) <30> Len:6 <06>
 *				Context Specific [4 0x04] <84> Len:4 <04>
 *					<31 38 30 33>
 *			NULL(5 0x05) <05> Len:0 <00>
 */
	0x91,
	0xA1, 0x18,
		0x02, 0x01,
			0x44,
		0x02, 0x01,
			0x07,
		0x30, 0x10,
			0x0A, 0x01,
				0x01,
			0x0A, 0x01,
				0x05,
			0x30, 0x06,
				0x84, 0x04,
					0x31, 0x38, 0x30, 0x33,
			0x05, 0x00,
	0x00, 0x00
/* *INDENT-ON* */
};


static const struct rose_message rose_qsig_msgs[] = {
/* *INDENT-OFF* */
	/* Q.SIG Name-Operations */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallingName,
		.component.invoke.invoke_id = 2,
		.component.invoke.args.qsig.CallingName.name.presentation = 1,
		.component.invoke.args.qsig.CallingName.name.char_set = 1,
		.component.invoke.args.qsig.CallingName.name.length = 7,
		.component.invoke.args.qsig.CallingName.name.data = "Alphred",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallingName,
		.component.invoke.invoke_id = 3,
		.component.invoke.args.qsig.CallingName.name.presentation = 1,
		.component.invoke.args.qsig.CallingName.name.char_set = 3,
		.component.invoke.args.qsig.CallingName.name.length = 7,
		.component.invoke.args.qsig.CallingName.name.data = "Alphred",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallingName,
		.component.invoke.invoke_id = 4,
		.component.invoke.args.qsig.CallingName.name.presentation = 2,
		.component.invoke.args.qsig.CallingName.name.char_set = 1,
		.component.invoke.args.qsig.CallingName.name.length = 7,
		.component.invoke.args.qsig.CallingName.name.data = "Alphred",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallingName,
		.component.invoke.invoke_id = 5,
		.component.invoke.args.qsig.CallingName.name.presentation = 2,
		.component.invoke.args.qsig.CallingName.name.char_set = 3,
		.component.invoke.args.qsig.CallingName.name.length = 7,
		.component.invoke.args.qsig.CallingName.name.data = "Alphred",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallingName,
		.component.invoke.invoke_id = 6,
		.component.invoke.args.qsig.CallingName.name.presentation = 3,
		.component.invoke.args.qsig.CallingName.name.char_set = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallingName,
		.component.invoke.invoke_id = 7,
		.component.invoke.args.qsig.CallingName.name.presentation = 4,
		.component.invoke.args.qsig.CallingName.name.char_set = 1,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CalledName,
		.component.invoke.invoke_id = 8,
		.component.invoke.args.qsig.CallingName.name.presentation = 4,
		.component.invoke.args.qsig.CallingName.name.char_set = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_ConnectedName,
		.component.invoke.invoke_id = 9,
		.component.invoke.args.qsig.CallingName.name.presentation = 4,
		.component.invoke.args.qsig.CallingName.name.char_set = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_BusyName,
		.component.invoke.invoke_id = 10,
		.component.invoke.args.qsig.CallingName.name.presentation = 4,
		.component.invoke.args.qsig.CallingName.name.char_set = 1,
	},

	/* Q.SIG SS-AOC-Operations */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_ChargeRequest,
		.component.invoke.invoke_id = 11,
		.component.invoke.args.qsig.ChargeRequest.num_records = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_ChargeRequest,
		.component.invoke.invoke_id = 12,
		.component.invoke.args.qsig.ChargeRequest.num_records = 1,
		.component.invoke.args.qsig.ChargeRequest.advice_mode_combinations[0] = 3,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_ChargeRequest,
		.component.invoke.invoke_id = 13,
		.component.invoke.args.qsig.ChargeRequest.num_records = 2,
		.component.invoke.args.qsig.ChargeRequest.advice_mode_combinations[0] = 4,
		.component.invoke.args.qsig.ChargeRequest.advice_mode_combinations[1] = 3,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_ChargeRequest,
		.component.result.invoke_id = 14,
		.component.result.args.qsig.ChargeRequest.advice_mode_combination = 3,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_GetFinalCharge,
		.component.invoke.invoke_id = 15,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocFinal,
		.component.invoke.invoke_id = 16,
		.component.invoke.args.qsig.AocFinal.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocFinal,
		.component.invoke.invoke_id = 17,
		.component.invoke.args.qsig.AocFinal.type = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocFinal,
		.component.invoke.invoke_id = 18,
		.component.invoke.args.qsig.AocFinal.type = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.currency = 800,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.multiplier = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.currency = "Rupies",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocFinal,
		.component.invoke.invoke_id = 19,
		.component.invoke.args.qsig.AocFinal.type = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.currency = 800,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.multiplier = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.currency = "Rupies",
		.component.invoke.args.qsig.AocFinal.specific.billing_id_present = 1,
		.component.invoke.args.qsig.AocFinal.specific.billing_id = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocFinal,
		.component.invoke.invoke_id = 20,
		.component.invoke.args.qsig.AocFinal.type = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.currency = 800,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.multiplier = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.currency = "Rupies",
		.component.invoke.args.qsig.AocFinal.charging_association_present = 1,
		.component.invoke.args.qsig.AocFinal.charging_association.type = 0,
		.component.invoke.args.qsig.AocFinal.charging_association.id = 200,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocFinal,
		.component.invoke.invoke_id = 21,
		.component.invoke.args.qsig.AocFinal.type = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.currency = 800,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.multiplier = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.currency = "Rupies",
		.component.invoke.args.qsig.AocFinal.specific.billing_id_present = 1,
		.component.invoke.args.qsig.AocFinal.specific.billing_id = 2,
		.component.invoke.args.qsig.AocFinal.charging_association_present = 1,
		.component.invoke.args.qsig.AocFinal.charging_association.type = 0,
		.component.invoke.args.qsig.AocFinal.charging_association.id = 200,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocFinal,
		.component.invoke.invoke_id = 22,
		.component.invoke.args.qsig.AocFinal.type = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.currency = 800,
		.component.invoke.args.qsig.AocFinal.specific.recorded.amount.multiplier = 2,
		.component.invoke.args.qsig.AocFinal.specific.recorded.currency = "Rupies",
		.component.invoke.args.qsig.AocFinal.charging_association_present = 1,
		.component.invoke.args.qsig.AocFinal.charging_association.type = 1,
		.component.invoke.args.qsig.AocFinal.charging_association.number.plan = 4,
		.component.invoke.args.qsig.AocFinal.charging_association.number.length = 4,
		.component.invoke.args.qsig.AocFinal.charging_association.number.str = "1802",
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocInterim,
		.component.invoke.invoke_id = 23,
		.component.invoke.args.qsig.AocInterim.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocInterim,
		.component.invoke.invoke_id = 24,
		.component.invoke.args.qsig.AocInterim.type = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocInterim,
		.component.invoke.invoke_id = 25,
		.component.invoke.args.qsig.AocInterim.type = 2,
		.component.invoke.args.qsig.AocInterim.specific.recorded.amount.currency = 800,
		.component.invoke.args.qsig.AocInterim.specific.recorded.amount.multiplier = 2,
		.component.invoke.args.qsig.AocInterim.specific.recorded.currency = "Rupies",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocInterim,
		.component.invoke.invoke_id = 26,
		.component.invoke.args.qsig.AocInterim.type = 2,
		.component.invoke.args.qsig.AocInterim.specific.recorded.amount.currency = 800,
		.component.invoke.args.qsig.AocInterim.specific.recorded.amount.multiplier = 2,
		.component.invoke.args.qsig.AocInterim.specific.recorded.currency = "Rupies",
		.component.invoke.args.qsig.AocInterim.specific.billing_id_present = 1,
		.component.invoke.args.qsig.AocInterim.specific.billing_id = 2,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 27,
		.component.invoke.args.qsig.AocRate.type = 0,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 28,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 0,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.special_charging_code = 3,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 29,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.currency = "Dollars",
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.amount.currency = 7,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.amount.multiplier = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.charging_type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.time.length = 8,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.time.scale = 4,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 30,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.currency = "Dollars",
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.amount.currency = 7,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.amount.multiplier = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.charging_type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.time.length = 8,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.time.scale = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.granularity_present = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.granularity.length = 20,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.duration.granularity.scale = 3,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 31,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 2,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.flat_rate.currency = "Euros",
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.flat_rate.amount.currency = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.flat_rate.amount.multiplier = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 32,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 3,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.volume_rate.currency = "Yen",
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.volume_rate.amount.currency = 300,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.volume_rate.amount.multiplier = 5,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.volume_rate.unit = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 33,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 2,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 2,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.flat_rate.currency = "Euros",
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.flat_rate.amount.currency = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].u.flat_rate.amount.multiplier = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[1].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[1].currency_type = 3,
		.component.invoke.args.qsig.AocRate.currency_info.list[1].u.volume_rate.currency = "Yen",
		.component.invoke.args.qsig.AocRate.currency_info.list[1].u.volume_rate.amount.currency = 300,
		.component.invoke.args.qsig.AocRate.currency_info.list[1].u.volume_rate.amount.multiplier = 5,
		.component.invoke.args.qsig.AocRate.currency_info.list[1].u.volume_rate.unit = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 34,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 4,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 35,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 5,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocRate,
		.component.invoke.invoke_id = 36,
		.component.invoke.args.qsig.AocRate.type = 1,
		.component.invoke.args.qsig.AocRate.currency_info.num_records = 1,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].charged_item = 4,
		.component.invoke.args.qsig.AocRate.currency_info.list[0].currency_type = 6,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocComplete,
		.component.invoke.invoke_id = 37,
		.component.invoke.args.qsig.AocComplete.charged_user_number.plan = 4,
		.component.invoke.args.qsig.AocComplete.charged_user_number.length = 4,
		.component.invoke.args.qsig.AocComplete.charged_user_number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocComplete,
		.component.invoke.invoke_id = 38,
		.component.invoke.args.qsig.AocComplete.charged_user_number.plan = 4,
		.component.invoke.args.qsig.AocComplete.charged_user_number.length = 4,
		.component.invoke.args.qsig.AocComplete.charged_user_number.str = "8340",
		.component.invoke.args.qsig.AocComplete.charging_association_present = 1,
		.component.invoke.args.qsig.AocComplete.charging_association.type = 0,
		.component.invoke.args.qsig.AocComplete.charging_association.id = 8298,
	},

	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_AocComplete,
		.component.result.invoke_id = 39,
		.component.result.args.qsig.AocComplete.charging_option = 2,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocDivChargeReq,
		.component.invoke.invoke_id = 40,
		.component.invoke.args.qsig.AocDivChargeReq.diverting_user_number.plan = 4,
		.component.invoke.args.qsig.AocDivChargeReq.diverting_user_number.length = 4,
		.component.invoke.args.qsig.AocDivChargeReq.diverting_user_number.str = "8340",
		.component.invoke.args.qsig.AocDivChargeReq.diversion_type = 3,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_AocDivChargeReq,
		.component.invoke.invoke_id = 41,
		.component.invoke.args.qsig.AocDivChargeReq.diverting_user_number.plan = 4,
		.component.invoke.args.qsig.AocDivChargeReq.diverting_user_number.length = 4,
		.component.invoke.args.qsig.AocDivChargeReq.diverting_user_number.str = "8340",
		.component.invoke.args.qsig.AocDivChargeReq.charging_association_present = 1,
		.component.invoke.args.qsig.AocDivChargeReq.charging_association.type = 0,
		.component.invoke.args.qsig.AocDivChargeReq.charging_association.id = 8298,
		.component.invoke.args.qsig.AocDivChargeReq.diversion_type = 3,
	},

	/* Q.SIG Call-Transfer-Operations (CT) */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferIdentify,
		.component.invoke.invoke_id = 42,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_CallTransferIdentify,
		.component.result.invoke_id = 43,
		.component.result.args.qsig.CallTransferIdentify.call_id = "2345",
		.component.result.args.qsig.CallTransferIdentify.rerouting_number.plan = 4,
		.component.result.args.qsig.CallTransferIdentify.rerouting_number.length = 4,
		.component.result.args.qsig.CallTransferIdentify.rerouting_number.str = "8340",
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferAbandon,
		.component.invoke.invoke_id = 44,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferInitiate,
		.component.invoke.invoke_id = 45,
		.component.invoke.args.qsig.CallTransferInitiate.call_id = "2345",
		.component.invoke.args.qsig.CallTransferInitiate.rerouting_number.plan = 4,
		.component.invoke.args.qsig.CallTransferInitiate.rerouting_number.length = 4,
		.component.invoke.args.qsig.CallTransferInitiate.rerouting_number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_CallTransferInitiate,
		.component.result.invoke_id = 46,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferSetup,
		.component.invoke.invoke_id = 47,
		.component.invoke.args.qsig.CallTransferSetup.call_id = "23",
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_CallTransferSetup,
		.component.result.invoke_id = 48,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferActive,
		.component.invoke.invoke_id = 49,
		.component.invoke.args.qsig.CallTransferActive.connected.presentation = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferActive,
		.component.invoke.invoke_id = 50,
		.component.invoke.args.qsig.CallTransferActive.connected.presentation = 1,
		.component.invoke.args.qsig.CallTransferActive.q931ie.length = 2,
		.component.invoke.args.qsig.CallTransferActive.q931ie_contents = "RT",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferActive,
		.component.invoke.invoke_id = 51,
		.component.invoke.args.qsig.CallTransferActive.connected.presentation = 1,
		.component.invoke.args.qsig.CallTransferActive.connected_name_present = 1,
		.component.invoke.args.qsig.CallTransferActive.connected_name.presentation = 1,
		.component.invoke.args.qsig.CallTransferActive.connected_name.char_set = 1,
		.component.invoke.args.qsig.CallTransferActive.connected_name.length = 7,
		.component.invoke.args.qsig.CallTransferActive.connected_name.data = "Alphred",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferActive,
		.component.invoke.invoke_id = 52,
		.component.invoke.args.qsig.CallTransferActive.connected.presentation = 1,
		.component.invoke.args.qsig.CallTransferActive.q931ie.length = 2,
		.component.invoke.args.qsig.CallTransferActive.q931ie_contents = "RT",
		.component.invoke.args.qsig.CallTransferActive.connected_name_present = 1,
		.component.invoke.args.qsig.CallTransferActive.connected_name.presentation = 1,
		.component.invoke.args.qsig.CallTransferActive.connected_name.char_set = 1,
		.component.invoke.args.qsig.CallTransferActive.connected_name.length = 7,
		.component.invoke.args.qsig.CallTransferActive.connected_name.data = "Alphred",
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 53,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 0,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.screening_indicator = 3,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.number.plan = 4,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.number.length = 4,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 54,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 55,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 56,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 3,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.screening_indicator = 3,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.number.plan = 4,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.number.length = 4,
		.component.invoke.args.qsig.CallTransferComplete.redirection.screened.number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 57,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 2,
		.component.invoke.args.qsig.CallTransferComplete.q931ie.length = 2,
		.component.invoke.args.qsig.CallTransferComplete.q931ie_contents = "RT",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 58,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 2,
		.component.invoke.args.qsig.CallTransferComplete.redirection_name_present = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection_name.presentation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection_name.char_set = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection_name.length = 7,
		.component.invoke.args.qsig.CallTransferComplete.redirection_name.data = "Alphred",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 59,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 2,
		.component.invoke.args.qsig.CallTransferComplete.call_status = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferComplete,
		.component.invoke.invoke_id = 60,
		.component.invoke.args.qsig.CallTransferComplete.end_designation = 1,
		.component.invoke.args.qsig.CallTransferComplete.redirection.presentation = 2,
		.component.invoke.args.qsig.CallTransferComplete.q931ie.length = 2,
		.component.invoke.args.qsig.CallTransferComplete.q931ie_contents = "RT",
		.component.invoke.args.qsig.CallTransferComplete.call_status = 1,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferUpdate,
		.component.invoke.invoke_id = 61,
		.component.invoke.args.qsig.CallTransferUpdate.redirection.presentation = 2,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferUpdate,
		.component.invoke.invoke_id = 62,
		.component.invoke.args.qsig.CallTransferUpdate.redirection.presentation = 2,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name_present = 1,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.presentation = 1,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.char_set = 1,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.length = 7,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.data = "Alphred",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferUpdate,
		.component.invoke.invoke_id = 63,
		.component.invoke.args.qsig.CallTransferUpdate.redirection.presentation = 2,
		.component.invoke.args.qsig.CallTransferUpdate.q931ie.length = 2,
		.component.invoke.args.qsig.CallTransferUpdate.q931ie_contents = "RT",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallTransferUpdate,
		.component.invoke.invoke_id = 64,
		.component.invoke.args.qsig.CallTransferUpdate.redirection.presentation = 2,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name_present = 1,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.presentation = 1,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.char_set = 1,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.length = 7,
		.component.invoke.args.qsig.CallTransferUpdate.redirection_name.data = "Alphred",
		.component.invoke.args.qsig.CallTransferUpdate.q931ie.length = 2,
		.component.invoke.args.qsig.CallTransferUpdate.q931ie_contents = "RT",
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_SubaddressTransfer,
		.component.invoke.invoke_id = 65,
		.component.invoke.args.qsig.SubaddressTransfer.redirection_subaddress.type = 1,
		.component.invoke.args.qsig.SubaddressTransfer.redirection_subaddress.length = 4,
		.component.invoke.args.qsig.SubaddressTransfer.redirection_subaddress.u.nsap = "4356",
	},

	/* Q.SIG Call-Diversion-Operations */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_ActivateDiversionQ,
		.component.invoke.invoke_id = 66,
		.component.invoke.args.qsig.ActivateDiversionQ.procedure = 1,
		.component.invoke.args.qsig.ActivateDiversionQ.basic_service = 3,
		.component.invoke.args.qsig.ActivateDiversionQ.diverted_to.number.plan = 4,
		.component.invoke.args.qsig.ActivateDiversionQ.diverted_to.number.length = 4,
		.component.invoke.args.qsig.ActivateDiversionQ.diverted_to.number.str = "8340",
		.component.invoke.args.qsig.ActivateDiversionQ.served_user_number.plan = 4,
		.component.invoke.args.qsig.ActivateDiversionQ.served_user_number.length = 4,
		.component.invoke.args.qsig.ActivateDiversionQ.served_user_number.str = "8340",
		.component.invoke.args.qsig.ActivateDiversionQ.activating_user_number.plan = 4,
		.component.invoke.args.qsig.ActivateDiversionQ.activating_user_number.length = 4,
		.component.invoke.args.qsig.ActivateDiversionQ.activating_user_number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_ActivateDiversionQ,
		.component.result.invoke_id = 67,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_DeactivateDiversionQ,
		.component.invoke.invoke_id = 68,
		.component.invoke.args.qsig.DeactivateDiversionQ.procedure = 1,
		.component.invoke.args.qsig.DeactivateDiversionQ.basic_service = 3,
		.component.invoke.args.qsig.DeactivateDiversionQ.served_user_number.plan = 4,
		.component.invoke.args.qsig.DeactivateDiversionQ.served_user_number.length = 4,
		.component.invoke.args.qsig.DeactivateDiversionQ.served_user_number.str = "8340",
		.component.invoke.args.qsig.DeactivateDiversionQ.deactivating_user_number.plan = 4,
		.component.invoke.args.qsig.DeactivateDiversionQ.deactivating_user_number.length = 4,
		.component.invoke.args.qsig.DeactivateDiversionQ.deactivating_user_number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_DeactivateDiversionQ,
		.component.result.invoke_id = 69,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_InterrogateDiversionQ,
		.component.invoke.invoke_id = 70,
		.component.invoke.args.qsig.InterrogateDiversionQ.procedure = 1,
		.component.invoke.args.qsig.InterrogateDiversionQ.basic_service = 3,
		.component.invoke.args.qsig.InterrogateDiversionQ.served_user_number.plan = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.served_user_number.length = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.served_user_number.str = "8340",
		.component.invoke.args.qsig.InterrogateDiversionQ.interrogating_user_number.plan = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.interrogating_user_number.length = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.interrogating_user_number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_InterrogateDiversionQ,
		.component.invoke.invoke_id = 71,
		.component.invoke.args.qsig.InterrogateDiversionQ.procedure = 1,
		.component.invoke.args.qsig.InterrogateDiversionQ.basic_service = 0,/* default */
		.component.invoke.args.qsig.InterrogateDiversionQ.served_user_number.plan = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.served_user_number.length = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.served_user_number.str = "8340",
		.component.invoke.args.qsig.InterrogateDiversionQ.interrogating_user_number.plan = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.interrogating_user_number.length = 4,
		.component.invoke.args.qsig.InterrogateDiversionQ.interrogating_user_number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_InterrogateDiversionQ,
		.component.result.invoke_id = 72,
		.component.result.args.qsig.InterrogateDiversionQ.num_records = 0,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_InterrogateDiversionQ,
		.component.result.invoke_id = 73,
		.component.result.args.qsig.InterrogateDiversionQ.num_records = 1,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[0].basic_service = 3,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].procedure = 2,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[0].remote_enabled = 0,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_InterrogateDiversionQ,
		.component.result.invoke_id = 74,
		.component.result.args.qsig.InterrogateDiversionQ.num_records = 1,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[0].basic_service = 3,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].procedure = 2,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[0].remote_enabled = 1,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_InterrogateDiversionQ,
		.component.result.invoke_id = 75,
		.component.result.args.qsig.InterrogateDiversionQ.num_records = 2,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].served_user_number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[0].basic_service = 3,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].procedure = 2,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[0].diverted_to.number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[1].served_user_number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[1].served_user_number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[1].served_user_number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[1].basic_service = 3,
		.component.result.args.qsig.InterrogateDiversionQ.list[1].procedure = 2,
		.component.result.args.qsig.InterrogateDiversionQ.list[1].diverted_to.number.plan = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[1].diverted_to.number.length = 4,
		.component.result.args.qsig.InterrogateDiversionQ.list[1].diverted_to.number.str = "8340",
		.component.result.args.qsig.InterrogateDiversionQ.list[1].remote_enabled = 1,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CheckRestriction,
		.component.invoke.invoke_id = 76,
		.component.invoke.args.qsig.CheckRestriction.served_user_number.plan = 4,
		.component.invoke.args.qsig.CheckRestriction.served_user_number.length = 4,
		.component.invoke.args.qsig.CheckRestriction.served_user_number.str = "8340",
		.component.invoke.args.qsig.CheckRestriction.basic_service = 3,
		.component.invoke.args.qsig.CheckRestriction.diverted_to_number.plan = 4,
		.component.invoke.args.qsig.CheckRestriction.diverted_to_number.length = 4,
		.component.invoke.args.qsig.CheckRestriction.diverted_to_number.str = "8340",
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_CheckRestriction,
		.component.result.invoke_id = 77,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallRerouting,
		.component.invoke.invoke_id = 78,
		.component.invoke.args.qsig.CallRerouting.rerouting_reason = 3,
		.component.invoke.args.qsig.CallRerouting.called.number.plan = 4,
		.component.invoke.args.qsig.CallRerouting.called.number.length = 4,
		.component.invoke.args.qsig.CallRerouting.called.number.str = "8340",
		.component.invoke.args.qsig.CallRerouting.diversion_counter = 5,
		.component.invoke.args.qsig.CallRerouting.q931ie.length = 2,
		.component.invoke.args.qsig.CallRerouting.q931ie_contents = "RT",
		.component.invoke.args.qsig.CallRerouting.last_rerouting.presentation = 1,
		.component.invoke.args.qsig.CallRerouting.subscription_option = 2,
		.component.invoke.args.qsig.CallRerouting.calling.presentation = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CallRerouting,
		.component.invoke.invoke_id = 79,
		.component.invoke.args.qsig.CallRerouting.rerouting_reason = 3,
		.component.invoke.args.qsig.CallRerouting.original_rerouting_reason_present = 1,
		.component.invoke.args.qsig.CallRerouting.original_rerouting_reason = 2,
		.component.invoke.args.qsig.CallRerouting.called.number.plan = 4,
		.component.invoke.args.qsig.CallRerouting.called.number.length = 4,
		.component.invoke.args.qsig.CallRerouting.called.number.str = "8340",
		.component.invoke.args.qsig.CallRerouting.diversion_counter = 5,
		.component.invoke.args.qsig.CallRerouting.q931ie.length = 2,
		.component.invoke.args.qsig.CallRerouting.q931ie_contents = "RT",
		.component.invoke.args.qsig.CallRerouting.last_rerouting.presentation = 1,
		.component.invoke.args.qsig.CallRerouting.subscription_option = 2,
		.component.invoke.args.qsig.CallRerouting.calling_subaddress.type = 1,
		.component.invoke.args.qsig.CallRerouting.calling_subaddress.length = 4,
		.component.invoke.args.qsig.CallRerouting.calling_subaddress.u.nsap = "3253",
		.component.invoke.args.qsig.CallRerouting.calling.presentation = 1,
		.component.invoke.args.qsig.CallRerouting.calling_name_present = 1,
		.component.invoke.args.qsig.CallRerouting.calling_name.presentation = 4,
		.component.invoke.args.qsig.CallRerouting.calling_name.char_set = 1,
		.component.invoke.args.qsig.CallRerouting.original_called_present = 1,
		.component.invoke.args.qsig.CallRerouting.original_called.presentation = 2,
		.component.invoke.args.qsig.CallRerouting.redirecting_name_present = 1,
		.component.invoke.args.qsig.CallRerouting.redirecting_name.presentation = 4,
		.component.invoke.args.qsig.CallRerouting.redirecting_name.char_set = 1,
		.component.invoke.args.qsig.CallRerouting.original_called_name_present = 1,
		.component.invoke.args.qsig.CallRerouting.original_called_name.presentation = 4,
		.component.invoke.args.qsig.CallRerouting.original_called_name.char_set = 1,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_CallRerouting,
		.component.result.invoke_id = 80,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_DivertingLegInformation1,
		.component.invoke.invoke_id = 81,
		.component.invoke.args.qsig.DivertingLegInformation1.diversion_reason = 3,
		.component.invoke.args.qsig.DivertingLegInformation1.subscription_option = 1,
		.component.invoke.args.qsig.DivertingLegInformation1.nominated_number.plan = 4,
		.component.invoke.args.qsig.DivertingLegInformation1.nominated_number.length = 4,
		.component.invoke.args.qsig.DivertingLegInformation1.nominated_number.str = "8340",
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_DivertingLegInformation2,
		.component.invoke.invoke_id = 82,
		.component.invoke.args.qsig.DivertingLegInformation2.diversion_counter = 6,
		.component.invoke.args.qsig.DivertingLegInformation2.diversion_reason = 3,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_DivertingLegInformation2,
		.component.invoke.invoke_id = 83,
		.component.invoke.args.qsig.DivertingLegInformation2.diversion_counter = 6,
		.component.invoke.args.qsig.DivertingLegInformation2.diversion_reason = 3,
		.component.invoke.args.qsig.DivertingLegInformation2.original_diversion_reason_present = 1,
		.component.invoke.args.qsig.DivertingLegInformation2.original_diversion_reason = 2,
		.component.invoke.args.qsig.DivertingLegInformation2.diverting_present = 1,
		.component.invoke.args.qsig.DivertingLegInformation2.diverting.presentation = 2,
		.component.invoke.args.qsig.DivertingLegInformation2.original_called_present = 1,
		.component.invoke.args.qsig.DivertingLegInformation2.original_called.presentation = 2,
		.component.invoke.args.qsig.DivertingLegInformation2.redirecting_name_present = 1,
		.component.invoke.args.qsig.DivertingLegInformation2.redirecting_name.presentation = 4,
		.component.invoke.args.qsig.DivertingLegInformation2.redirecting_name.char_set = 1,
		.component.invoke.args.qsig.DivertingLegInformation2.original_called_name_present = 1,
		.component.invoke.args.qsig.DivertingLegInformation2.original_called_name.presentation = 4,
		.component.invoke.args.qsig.DivertingLegInformation2.original_called_name.char_set = 1,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_DivertingLegInformation3,
		.component.invoke.invoke_id = 84,
		.component.invoke.args.qsig.DivertingLegInformation3.presentation_allowed_indicator = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_DivertingLegInformation3,
		.component.invoke.invoke_id = 85,
		.component.invoke.args.qsig.DivertingLegInformation3.presentation_allowed_indicator = 1,
		.component.invoke.args.qsig.DivertingLegInformation3.redirection_name_present = 1,
		.component.invoke.args.qsig.DivertingLegInformation3.redirection_name.presentation = 4,
		.component.invoke.args.qsig.DivertingLegInformation3.redirection_name.char_set = 1,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_CfnrDivertedLegFailed,
		.component.invoke.invoke_id = 86,
	},

	/* Q.SIG SS-MWI-Operations */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIActivate,
		.component.invoke.invoke_id = 102,
		.component.invoke.args.qsig.MWIActivate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIActivate.basic_service = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIActivate,
		.component.invoke.invoke_id = 103,
		.component.invoke.args.qsig.MWIActivate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIActivate.basic_service = 1,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id_present = 1,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.type = 0,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.u.integer = 532,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIActivate,
		.component.invoke.invoke_id = 104,
		.component.invoke.args.qsig.MWIActivate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIActivate.basic_service = 1,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id_present = 1,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.type = 1,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.u.number.plan = 4,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.u.number.length = 4,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.u.number.str = "9838",
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIActivate,
		.component.invoke.invoke_id = 105,
		.component.invoke.args.qsig.MWIActivate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIActivate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIActivate.basic_service = 1,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id_present = 1,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.type = 2,
		.component.invoke.args.qsig.MWIActivate.msg_centre_id.u.str = "123456",
		.component.invoke.args.qsig.MWIActivate.number_of_messages_present = 1,
		.component.invoke.args.qsig.MWIActivate.number_of_messages = 6548,
		.component.invoke.args.qsig.MWIActivate.originating_number.plan = 4,
		.component.invoke.args.qsig.MWIActivate.originating_number.length = 4,
		.component.invoke.args.qsig.MWIActivate.originating_number.str = "9838",
		.component.invoke.args.qsig.MWIActivate.timestamp_present = 1,
		.component.invoke.args.qsig.MWIActivate.timestamp = "19970621194530",
		.component.invoke.args.qsig.MWIActivate.priority_present = 1,
		.component.invoke.args.qsig.MWIActivate.priority = 7,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_MWIActivate,
		.component.result.invoke_id = 106,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIDeactivate,
		.component.invoke.invoke_id = 107,
		.component.invoke.args.qsig.MWIDeactivate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIDeactivate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIDeactivate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIDeactivate.basic_service = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIDeactivate,
		.component.invoke.invoke_id = 108,
		.component.invoke.args.qsig.MWIDeactivate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIDeactivate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIDeactivate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIDeactivate.basic_service = 1,
		.component.invoke.args.qsig.MWIDeactivate.msg_centre_id_present = 1,
		.component.invoke.args.qsig.MWIDeactivate.msg_centre_id.type = 0,
		.component.invoke.args.qsig.MWIDeactivate.msg_centre_id.u.integer = 532,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_MWIDeactivate,
		.component.result.invoke_id = 109,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIInterrogate,
		.component.invoke.invoke_id = 110,
		.component.invoke.args.qsig.MWIInterrogate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIInterrogate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIInterrogate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIInterrogate.basic_service = 1,
	},
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_QSIG_MWIInterrogate,
		.component.invoke.invoke_id = 111,
		.component.invoke.args.qsig.MWIInterrogate.served_user_number.plan = 4,
		.component.invoke.args.qsig.MWIInterrogate.served_user_number.length = 4,
		.component.invoke.args.qsig.MWIInterrogate.served_user_number.str = "9838",
		.component.invoke.args.qsig.MWIInterrogate.basic_service = 1,
		.component.invoke.args.qsig.MWIInterrogate.msg_centre_id_present = 1,
		.component.invoke.args.qsig.MWIInterrogate.msg_centre_id.type = 0,
		.component.invoke.args.qsig.MWIInterrogate.msg_centre_id.u.integer = 532,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_MWIInterrogate,
		.component.result.invoke_id = 112,
		.component.result.args.qsig.MWIInterrogate.num_records = 1,
		.component.result.args.qsig.MWIInterrogate.list[0].basic_service = 1,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_QSIG_MWIInterrogate,
		.component.result.invoke_id = 113,
		.component.result.args.qsig.MWIInterrogate.num_records = 2,
		.component.result.args.qsig.MWIInterrogate.list[0].basic_service = 1,
		.component.result.args.qsig.MWIInterrogate.list[0].msg_centre_id_present = 1,
		.component.result.args.qsig.MWIInterrogate.list[0].msg_centre_id.type = 0,
		.component.result.args.qsig.MWIInterrogate.list[0].msg_centre_id.u.integer = 987,
		.component.result.args.qsig.MWIInterrogate.list[0].number_of_messages_present = 1,
		.component.result.args.qsig.MWIInterrogate.list[0].number_of_messages = 6548,
		.component.result.args.qsig.MWIInterrogate.list[0].originating_number.plan = 4,
		.component.result.args.qsig.MWIInterrogate.list[0].originating_number.length = 4,
		.component.result.args.qsig.MWIInterrogate.list[0].originating_number.str = "9838",
		.component.result.args.qsig.MWIInterrogate.list[0].timestamp_present = 1,
		.component.result.args.qsig.MWIInterrogate.list[0].timestamp = "19970621194530",
		.component.result.args.qsig.MWIInterrogate.list[0].priority_present = 1,
		.component.result.args.qsig.MWIInterrogate.list[0].priority = 7,
		.component.result.args.qsig.MWIInterrogate.list[1].basic_service = 1,
	},
/* *INDENT-ON* */
};


static const struct rose_message rose_dms100_msgs[] = {
/* *INDENT-OFF* */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_DMS100_RLT_OperationInd,
		.component.invoke.invoke_id = ROSE_DMS100_RLT_OPERATION_IND,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_DMS100_RLT_OperationInd,
		.component.result.invoke_id = ROSE_DMS100_RLT_OPERATION_IND,
		.component.result.args.dms100.RLT_OperationInd.call_id = 130363,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_DMS100_RLT_ThirdParty,
		.component.invoke.invoke_id = ROSE_DMS100_RLT_THIRD_PARTY,
		.component.invoke.args.dms100.RLT_ThirdParty.call_id = 120047,
		.component.invoke.args.dms100.RLT_ThirdParty.reason = 1,
	},
	{
		.type = ROSE_COMP_TYPE_RESULT,
		.component.result.operation = ROSE_DMS100_RLT_ThirdParty,
		.component.result.invoke_id = ROSE_DMS100_RLT_THIRD_PARTY,
	},
/* *INDENT-ON* */
};


static const struct rose_message rose_ni2_msgs[] = {
/* *INDENT-OFF* */
	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_NI2_InformationFollowing,
		.component.invoke.invoke_id = 1,
		.component.invoke.args.ni2.InformationFollowing.value = 7,
	},

	{
		.type = ROSE_COMP_TYPE_INVOKE,
		.component.invoke.operation = ROSE_NI2_InitiateTransfer,
		.component.invoke.invoke_id = 2,
		.component.invoke.args.ni2.InitiateTransfer.call_reference = 5,
	},
/* *INDENT-ON* */
};

/* ------------------------------------------------------------------- */

static void rose_pri_message(struct pri *ctrl, char *stuff)
{
	fprintf(stdout, "%s", stuff);
}

static void rose_pri_error(struct pri *ctrl, char *stuff)
{
	fprintf(stdout, "%s", stuff);
	fprintf(stderr, "%s", stuff);
}

/*!
 * \internal
 * \brief Test ROSE encoding and decoding the given message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param index Message number to report.
 * \param header Facility message header data to encode.
 * \param encode_msg Message data to encode.
 *
 * \return Nothing
 */
static void rose_test_msg(struct pri *ctrl, unsigned index,
	const struct fac_extension_header *header, const struct rose_message *encode_msg)
{
	struct fac_extension_header decoded_header;
	struct rose_message decoded_msg;
	unsigned char *enc_pos;
	unsigned char *enc_end;
	const unsigned char *dec_pos;
	const unsigned char *dec_end;

	static unsigned char buf[1024];

	pri_message(ctrl, "\n\n");
	enc_end = buf + sizeof(buf);
	enc_pos = facility_encode_header(ctrl, buf, enc_end, header);
	if (!enc_pos) {
		pri_error(ctrl, "Error: Message:%u failed to encode header\n", index);
	} else {
		enc_pos = rose_encode(ctrl, enc_pos, enc_end, encode_msg);
		if (!enc_pos) {
			pri_error(ctrl, "Error: Message:%u failed to encode ROSE\n", index);
		} else {
			pri_message(ctrl, "Message %u encoded length is %u\n", index,
				(unsigned) (enc_pos - buf));

			/* Clear the decoded message contents for comparison. */
			memset(&decoded_header, 0, sizeof(decoded_header));
			memset(&decoded_msg, 0, sizeof(decoded_msg));

			dec_end = enc_pos;
			dec_pos = facility_decode_header(ctrl, buf, dec_end, &decoded_header);
			if (!dec_pos) {
				pri_error(ctrl, "Error: Message:%u failed to decode header\n", index);
			} else {
				dec_pos = rose_decode(ctrl, dec_pos, dec_end, &decoded_msg);
				if (!dec_pos) {
					pri_error(ctrl, "Error: Message:%u failed to decode ROSE\n", index);
				} else {
					if (header
						&& memcmp(header, &decoded_header, sizeof(decoded_header))) {
						pri_error(ctrl, "Error: Message:%u Header did not match\n",
							index);
					}
					if (memcmp(encode_msg, &decoded_msg, sizeof(decoded_msg))) {
						pri_error(ctrl, "Error: Message:%u ROSE did not match\n", index);
					}
				}
			}
		}
	}
	pri_message(ctrl, "\n\n"
		"************************************************************\n");
}

/*!
 * \internal
 * \brief Test ROSE decoding messages of unusual encodings.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param name Test name for the encoded message.
 * \param msg_buf Encoded message to decode.
 * \param msg_len Length of encoded message buffer.
 *
 * \return Nothing
 */
static void rose_test_exception(struct pri *ctrl, const char *name,
	const unsigned char *msg, size_t msg_len)
{
	const unsigned char *pos;
	const unsigned char *end;
	struct fac_extension_header header;
	struct rose_message decoded_msg;

	pri_message(ctrl, "\n\n"
		"%s test: Message encoded length is %u\n", name, (unsigned) msg_len);

	pos = msg;
	end = msg + msg_len;
	pos = facility_decode_header(ctrl, pos, end, &header);
	if (!pos) {
		pri_error(ctrl, "Error: %s test: Message failed to decode header\n", name);
	} else {
		pos = rose_decode(ctrl, pos, end, &decoded_msg);
		if (!pos) {
			pri_error(ctrl, "Error: %s test: Message failed to decode ROSE\n", name);
		}
	}

	pri_message(ctrl, "\n\n"
		"************************************************************\n");
}

/*!
 * \brief ROSE encode/decode test program.
 *
 * \param argc Program argument count.
 * \param argv Program argument string array.
 *
 * \retval 0 on success.
 * \retval Nonzero on error.
 */
int main(int argc, char *argv[])
{
	unsigned index;
	unsigned offset;
	static struct pri dummy_ctrl;

	pri_set_message(rose_pri_message);
	pri_set_error(rose_pri_error);

	memset(&dummy_ctrl, 0, sizeof(dummy_ctrl));
	dummy_ctrl.debug = PRI_DEBUG_APDU;

	offset = 0;
	pri_message(&dummy_ctrl, "Encode/decode message(s)\n");
	if (argc <= 1) {
		dummy_ctrl.switchtype = PRI_SWITCH_EUROISDN_E1;
		for (index = 0; index < ARRAY_LEN(rose_etsi_msgs); ++index) {
			rose_test_msg(&dummy_ctrl, index + offset, &fac_headers[0],
				&rose_etsi_msgs[index]);
		}
		offset += ARRAY_LEN(rose_etsi_msgs);

		dummy_ctrl.switchtype = PRI_SWITCH_QSIG;
		for (index = 0; index < ARRAY_LEN(rose_qsig_msgs); ++index) {
			rose_test_msg(&dummy_ctrl, index + offset,
				&fac_headers[index % ARRAY_LEN(fac_headers)], &rose_qsig_msgs[index]);
		}
		offset += ARRAY_LEN(rose_qsig_msgs);

		dummy_ctrl.switchtype = PRI_SWITCH_DMS100;
		for (index = 0; index < ARRAY_LEN(rose_dms100_msgs); ++index) {
			rose_test_msg(&dummy_ctrl, index + offset, &fac_headers[0],
				&rose_dms100_msgs[index]);
		}
		offset += ARRAY_LEN(rose_dms100_msgs);

		dummy_ctrl.switchtype = PRI_SWITCH_NI2;
		for (index = 0; index < ARRAY_LEN(rose_ni2_msgs); ++index) {
			rose_test_msg(&dummy_ctrl, index + offset, &fac_headers[0],
				&rose_ni2_msgs[index]);
		}
		//offset += ARRAY_LEN(rose_ni2_msgs);
	} else {
		index = atoi(argv[1]);

		if (index < ARRAY_LEN(rose_etsi_msgs)) {
			dummy_ctrl.switchtype = PRI_SWITCH_EUROISDN_E1;
			rose_test_msg(&dummy_ctrl, index + offset, &fac_headers[0],
				&rose_etsi_msgs[index]);
			return 0;
		}
		offset += ARRAY_LEN(rose_etsi_msgs);
		index -= ARRAY_LEN(rose_etsi_msgs);

		if (index < ARRAY_LEN(rose_qsig_msgs)) {
			dummy_ctrl.switchtype = PRI_SWITCH_QSIG;
			rose_test_msg(&dummy_ctrl, index + offset,
				&fac_headers[index % ARRAY_LEN(fac_headers)], &rose_qsig_msgs[index]);
			return 0;
		}
		offset += ARRAY_LEN(rose_qsig_msgs);
		index -= ARRAY_LEN(rose_qsig_msgs);

		if (index < ARRAY_LEN(rose_dms100_msgs)) {
			dummy_ctrl.switchtype = PRI_SWITCH_DMS100;
			rose_test_msg(&dummy_ctrl, index + offset, &fac_headers[0],
				&rose_dms100_msgs[index]);
			return 0;
		}
		offset += ARRAY_LEN(rose_dms100_msgs);
		index -= ARRAY_LEN(rose_dms100_msgs);

		if (index < ARRAY_LEN(rose_ni2_msgs)) {
			dummy_ctrl.switchtype = PRI_SWITCH_NI2;
			rose_test_msg(&dummy_ctrl, index + offset, &fac_headers[0],
				&rose_ni2_msgs[index]);
			return 0;
		}
		//offset += ARRAY_LEN(rose_ni2_msgs);
		//index -= ARRAY_LEN(rose_ni2_msgs);

		fprintf(stderr, "Invalid option\n");
		return 0;
	}

/* ------------------------------------------------------------------- */

	pri_message(&dummy_ctrl, "\n\n"
		"Decode unusually encoded messages\n");

	dummy_ctrl.switchtype = PRI_SWITCH_EUROISDN_E1;

	rose_test_exception(&dummy_ctrl, "Extra bytes on end", rose_etsi_extra,
		sizeof(rose_etsi_extra));

	rose_test_exception(&dummy_ctrl, "Indefinite length", rose_etsi_indefinite_len,
		sizeof(rose_etsi_indefinite_len) - 2);
	rose_test_exception(&dummy_ctrl, "Indefinite length (extra)",
		rose_etsi_indefinite_len, sizeof(rose_etsi_indefinite_len));

	rose_test_exception(&dummy_ctrl, "Unused components (indefinite length)",
		rose_etsi_unused_indefinite_len, sizeof(rose_etsi_unused_indefinite_len) - 2);
	rose_test_exception(&dummy_ctrl, "Unused components (indefinite length, extra)",
		rose_etsi_unused_indefinite_len, sizeof(rose_etsi_unused_indefinite_len));

	rose_test_exception(&dummy_ctrl, "Unused components", rose_etsi_unused,
		sizeof(rose_etsi_unused) - 2);
	rose_test_exception(&dummy_ctrl, "Unused components (extra)", rose_etsi_unused,
		sizeof(rose_etsi_unused));

/* ------------------------------------------------------------------- */

	pri_message(&dummy_ctrl, "\n\n"
		"List of operation codes:\n");
	for (index = 0; index < ROSE_Num_Operation_Codes; ++index) {
		pri_message(&dummy_ctrl, "%d: %s\n", index, rose_operation2str(index));
	}
	pri_message(&dummy_ctrl, "\n\n"
		"************************************************************\n");

/* ------------------------------------------------------------------- */

	pri_message(&dummy_ctrl, "\n\n"
		"List of error codes:\n");
	for (index = 0; index < ROSE_ERROR_Num_Codes; ++index) {
		pri_message(&dummy_ctrl, "%d: %s\n", index, rose_error2str(index));
	}
	pri_message(&dummy_ctrl, "\n\n"
		"************************************************************\n");

/* ------------------------------------------------------------------- */

	pri_message(&dummy_ctrl, "\n\n");
	pri_message(&dummy_ctrl, "sizeof(struct rose_message) = %u\n",
		(unsigned) sizeof(struct rose_message));
	pri_message(&dummy_ctrl, "sizeof(struct rose_msg_invoke) = %u\n",
		(unsigned) sizeof(struct rose_msg_invoke));
	pri_message(&dummy_ctrl, "sizeof(struct rose_msg_result) = %u\n",
		(unsigned) sizeof(struct rose_msg_result));
	pri_message(&dummy_ctrl, "sizeof(struct rose_msg_error) = %u\n",
		(unsigned) sizeof(struct rose_msg_error));
	pri_message(&dummy_ctrl, "sizeof(struct rose_msg_reject) = %u\n",
		(unsigned) sizeof(struct rose_msg_reject));
	pri_message(&dummy_ctrl, "sizeof(union rose_msg_invoke_args) = %u\n",
		(unsigned) sizeof(union rose_msg_invoke_args));
	pri_message(&dummy_ctrl, "sizeof(union rose_msg_result_args) = %u\n",
		(unsigned) sizeof(union rose_msg_result_args));

	pri_message(&dummy_ctrl, "\n");
	pri_message(&dummy_ctrl, "sizeof(struct roseQsigForwardingList) = %u\n",
		(unsigned) sizeof(struct roseQsigForwardingList));

	pri_message(&dummy_ctrl, "\n");
	pri_message(&dummy_ctrl, "sizeof(struct roseQsigCallRerouting_ARG) = %u\n",
		(unsigned) sizeof(struct roseQsigCallRerouting_ARG));
	pri_message(&dummy_ctrl, "sizeof(struct roseQsigAocRateArg_ARG) = %u\n",
		(unsigned) sizeof(struct roseQsigAocRateArg_ARG));
	pri_message(&dummy_ctrl, "sizeof(struct roseQsigMWIInterrogateRes) = %u\n",
		(unsigned) sizeof(struct roseQsigMWIInterrogateRes));

	pri_message(&dummy_ctrl, "\n");
	pri_message(&dummy_ctrl, "sizeof(struct roseEtsiAOCSCurrencyInfoList) = %u\n",
		(unsigned) sizeof(struct roseEtsiAOCSCurrencyInfoList));

/* ------------------------------------------------------------------- */

	return 0;
}

/* ------------------------------------------------------------------- */
/* end rosetest.c */
