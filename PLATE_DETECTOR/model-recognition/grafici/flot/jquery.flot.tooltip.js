/*
 * Flot plugin to show tooltips.
 * Copyright (c) 2010 - Andrea Piccinelli <frasten@gmail.com>
 * 
 * == Important notes ==
 * Requires grid.hoverable option set to true to work.
 * You can style the tooltip setting your custom CSS to the #tooltip
 * identifier.
 * 
 * 
 * == Default Options ==
 * tooltips: {
 *   show: false,
 *   displayfunc: function(item) {return item.datapoint[1]},
 *   fadeInTime: 150,
 *   id: "tooltip"
 * }
 * 
 * 
 * == Options ==
 * "show": set to true to enable tooltips
 * "displayFunc": a callback function, must return the text on the
 *   tooltip. 'item' is passed as a parameter, so you can retrieve the
 *   point data reading item.datapoint.
 * "fadeInTime": duration of the fade in, in millisec.
 * "id": the DOM id given to the tooltip.
 * 
 * 
 * == Example ==
$.plot($("#placeholder"),
  yourData,
  {
    tooltip: {
      show: true,
      displayfunc: function(item) {return "Value: " + item.datapoint[1]}
    }
  }
);

*/

(function ($) {
	var options = {
		tooltips: {
			show: false,
			displayFunc: function(item) {return item.datapoint[1]},
			fadeInTime: 150,
			id: "tooltip"
		}
	};

	function init(plot) {
		var previousPoint;

		function showTooltip(x, y, contents) {
			var opt = plot.getOptions().tooltips;
			$('<div id="' + opt.id + '">' + contents + '</div>').css({
				top: y + 5,
				left: x + 5,
				position: "absolute",
				display: "none"
			}).appendTo("body").fadeIn(opt.fadeInTime);
		}

		function MouseOverFunc(event, pos, item) {
			var opt = plot.getOptions().tooltips;
			if (!opt.show) return;
			if (item) {
				if (!previousPoint ||
				previousPoint[0] != item.datapoint[0] ||
				previousPoint[1] != item.datapoint[1]) {
					previousPoint = item.datapoint;
					// hover tooltip
					$("#" + opt.id).remove();
					showTooltip(item.pageX, item.pageY, opt.displayFunc(item))
				}
			}
			else {
				$("#" + opt.id).remove();
				previousPoint = null;
			}
		}

		plot.getPlaceholder().bind("plothover", MouseOverFunc);
	}

	$.plot.plugins.push({
		init: init,
		options: options,
		name: 'tooltip',
		version: '0.2'
	});
})(jQuery);
