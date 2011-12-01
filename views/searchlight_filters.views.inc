<?php

/**
 * Implements hook_views_data_alter().
 */
function searchlight_filters_views_data_alter(&$data) {
  foreach (searchlight_environment_load(NULL, TRUE) as $environment) {
    $data['searchlight'][$environment->name . '_filter'] = array(
      'title' => t('Filter for environment "!name"', array('!name' => $environment->name)),
      'help' => t('Filter results by active Searchlight facets.'),
      'filter' => array(
        'handler' => 'searchlight_filters_filter',
        'environment_name' => $environment->name,
      ),
    );
  }
}
